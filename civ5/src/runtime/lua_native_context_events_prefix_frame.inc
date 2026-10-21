// Inside Host::Impl after the completed-profile physical query. This is the
// real 5ACF20/60ECE0 protected registrar frame, distinct from LuaEvents700540.
struct NativeContextEventsPrefixFrame final {
    Impl& host;const LuaScriptHost& facade;
    const BnwWorldViewSourceOperationLoan& operation;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> prepared;
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan> name;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> state_name;
    const std::shared_ptr<const ui::detail::NotificationContextLuaProfileReturn> profile;
    const std::shared_ptr<const ui::detail::NotificationContextResizeRegistrationReturn> resize;
    const std::shared_ptr<const ScriptUiChildSource> source;
    const std::shared_ptr<const BnwContextLuaIncludeCollectionLoan> collection;
    const std::weak_ptr<const void> core;
    const std::thread::id creator=std::this_thread::get_id();
    const std::size_t child;const std::uint64_t serial,epoch,writes,uncertainty;
    const int environment,thread;const void*const actual_environment;lua_State*const actual_thread;
    // These are actual protected Lua allocations, never caller-supplied gates.
    int keys_ref=LUA_NOREF,master_ref=LUA_NOREF,factory_ref=LUA_NOREF,generators_ref=LUA_NOREF;
    const void* keys=nullptr;const void* master=nullptr;
    const void* factory=nullptr;const void* generators=nullptr;
    // Actual first75E540 closure allocations/assignment, never caller flags.
    const void* app_constructor_created=nullptr;const void* app_constructor_assigned=nullptr;
    const void* camera_constructor_created=nullptr;const void* camera_constructor_assigned=nullptr;
    const void* projection_constructor_created=nullptr;const void* projection_constructor_assigned=nullptr;
    const void* animation_constructor_created=nullptr;const void* animation_constructor_assigned=nullptr;
    const void* dont_record_constructor_created=nullptr;const void* dont_record_constructor_assigned=nullptr;
    const void* record_constructor_created=nullptr;const void* record_constructor_assigned=nullptr;
    const void* city_constructor_created=nullptr;const void* city_constructor_assigned=nullptr;
    const void* unit_constructor_created=nullptr;const void* unit_constructor_assigned=nullptr;
    const void* unit_type_constructor_created=nullptr;const void* unit_type_constructor_assigned=nullptr;
    const void* wonder_type_constructor_created=nullptr;const void* wonder_type_constructor_assigned=nullptr;
    const void* wonder_state_constructor_created=nullptr;const void* wonder_state_constructor_assigned=nullptr;
    const void* wonder_placement_constructor_created=nullptr;const void* wonder_placement_constructor_assigned=nullptr;
    const void* display_units_constructor_created=nullptr;const void* display_units_constructor_assigned=nullptr;
    const void* unit_scale_constructor_created=nullptr;const void* unit_scale_constructor_assigned=nullptr;
    const void* landmark_swap_constructor_created=nullptr;const void* landmark_swap_constructor_assigned=nullptr;
    const void* unit_swap_constructor_created=nullptr;const void* unit_swap_constructor_assigned=nullptr;
    const void* building_swap_constructor_created=nullptr;const void* building_swap_constructor_assigned=nullptr;
    const void* sequence_init_constructor_created=nullptr;const void* sequence_init_constructor_assigned=nullptr;
    const void* state_dump_constructor_created=nullptr;const void* state_dump_constructor_assigned=nullptr;
    const void* state_request_constructor_created=nullptr;const void* state_request_constructor_assigned=nullptr;
    const void* strategic_view_constructor_created=nullptr;const void* strategic_view_constructor_assigned=nullptr;
    const void* ai_started_constructor_created=nullptr;const void* ai_started_constructor_assigned=nullptr;
    const void* ai_ended_constructor_created=nullptr;const void* ai_ended_constructor_assigned=nullptr;
    const void* ai_step_constructor_created=nullptr;const void* ai_step_constructor_assigned=nullptr;
    const void* game_view_constructor_created=nullptr;const void* game_view_constructor_assigned=nullptr;
    std::size_t ordered_assigned_count=0; // actual successful original native rawset assignments
    const void* lua_event_constructor_created=nullptr;const void* lua_event_constructor_assigned=nullptr;
    const void* factory_index_created=nullptr;const void* factory_index_assigned=nullptr;
    const void* master_factory_assigned=nullptr;
    int published_events_reference=LUA_NOREF;const void* published_events_table=nullptr;
    NativeContextEventsPrefixFrame(Impl& h,const LuaScriptHost& f,const BnwWorldViewSourceOperationLoan& op,
      std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> p,
      std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan> n,
      std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> sn,
      std::shared_ptr<const ui::detail::NotificationContextLuaProfileReturn> pr,
      std::shared_ptr<const ui::detail::NotificationContextResizeRegistrationReturn> rr,
      std::shared_ptr<const ScriptUiChildSource> src,std::weak_ptr<const void> c,std::size_t slot,
      int env,int thr,const void* envptr,lua_State* thrptr)
      :host(h),facade(f),operation(op),prepared(std::move(p)),name(std::move(n)),state_name(std::move(sn)),
       profile(std::move(pr)),resize(std::move(rr)),source(std::move(src)),collection(h.profile_include_collection),core(std::move(c)),
       child(slot),serial(op.operation_serial()),epoch(h.failure_epoch),writes(h.failed.writes_),
       uncertainty(h.execution_uncertainty),environment(env),thread(thr),actual_environment(envptr),actual_thread(thrptr){
        if(host.events_prefix_frame)throw ScriptError("Events actual load frame is already entered");
        host.events_prefix_frame=this;
    }
    ~NativeContextEventsPrefixFrame(){if(host.events_prefix_frame==this)host.events_prefix_frame=nullptr;}
    void first()const {
        host.worldview_require_callback_origin();
        if(host.events_prefix_frame!=this||creator!=std::this_thread::get_id()||!host.worldview_native_host||
           host.worldview_native_host->loan!=&operation||host.worldview_native_host->child!=child||
           child>=host.child_contexts.size()||serial!=operation.operation_serial()||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||core.expired()||
           !prepared||!name||!state_name||!profile||!resize||!source||!collection||!actual_environment||!actual_thread||
           host.profile_include_collection.get()!=collection.get()||host.profile_include_collection.owner_before(collection)||
           collection.owner_before(host.profile_include_collection)||
           host.profile_gc_stopped)
            throw ScriptError("Events SAME completed-profile/load/Source frame changed");
        const auto& current=host.child_contexts[child];
        if(current.retired||!current.state_prepared||current.environment!=environment||current.execution_thread!=thread||
           current.source.get()!=source.get()||current.source.owner_before(source)||source.owner_before(current.source))
            throw ScriptError("Events actual child518/source/ref changed");
        const auto actual_name=current.setup_state_name_return.lock();
        const auto actual_profile=current.profile_completed_return.lock();
        if(actual_name.get()!=state_name.get()||actual_name.owner_before(state_name)||state_name.owner_before(actual_name)||
           actual_profile.get()!=profile.get()||actual_profile.owner_before(profile)||profile.owner_before(actual_profile))
            throw ScriptError("Events actual StateName/completed61 publication changed");
        name->require_current_without_callbacks(operation,prepared);
    }
    void full()const {
        first();operation.require_current();first();operation.require_same_vm(facade);first();
        operation.require_same_source(*source);first();name->require_current_owned(operation,prepared);first();
        name->require_context_resize_owned(operation,prepared,resize);first();
        collection->require_current_owned(operation);first();
        collection->require_current_without_callbacks(operation);first();
    }
};
