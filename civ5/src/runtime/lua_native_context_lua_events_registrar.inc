// Distinct actual658430 protected frame, entered only after real completed Events publication.
struct NativeContextLuaEventsFrame final {
    Impl& host;const LuaScriptHost& facade;
    const BnwWorldViewSourceOperationLoan& operation;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> prepared;
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan> name;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> state_name;
    const std::shared_ptr<const ScriptUiChildSource> source;
    const std::weak_ptr<const void> core;
    const NativeContextEventsPrefixFrame*const events; // captured actual live predecessor frame
    const std::thread::id creator=std::this_thread::get_id();
    const std::size_t child;const std::uint64_t serial,epoch,writes,uncertainty;
    const int environment,thread;const void*const actual_environment;lua_State*const actual_thread;
    NativeContextLuaEventsFrame(Impl& h,const LuaScriptHost& f,const BnwWorldViewSourceOperationLoan& op,
      std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> p,
      std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan> n,
      std::shared_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> sn,
      std::shared_ptr<const ScriptUiChildSource> src,std::weak_ptr<const void> c,std::size_t slot,
      int env,int thr,const void* envptr,lua_State* thrptr)
      :host(h),facade(f),operation(op),prepared(std::move(p)),name(std::move(n)),state_name(std::move(sn)),
       source(std::move(src)),core(std::move(c)),events(h.events_prefix_frame),
       child(slot),serial(op.operation_serial()),epoch(h.failure_epoch),writes(h.failed.writes_),
       uncertainty(h.execution_uncertainty),environment(env),thread(thr),actual_environment(envptr),actual_thread(thrptr){
        if(host.lua_events_frame)throw ScriptError("LuaEvents actual load frame is already entered");
        if(!events||events->child!=child||events->published_events_reference<0||!events->published_events_table||
           events->master_factory_assigned!=events->factory||!events->factory_index_assigned||
           !events->lua_event_constructor_assigned||events->ordered_assigned_count!=232)
            throw ScriptError("LuaEvents genuine Events4637C0 predecessor did not return");
        events->first();
        host.lua_events_frame=this;
    }
    ~NativeContextLuaEventsFrame(){if(host.lua_events_frame==this)host.lua_events_frame=nullptr;}
    void first()const {
        host.worldview_require_callback_origin();
        if(host.lua_events_frame!=this||creator!=std::this_thread::get_id()||!host.worldview_native_host||
           host.worldview_native_host->loan!=&operation||host.worldview_native_host->child!=child||
           child>=host.child_contexts.size()||serial!=operation.operation_serial()||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||core.expired()||
           !prepared||!name||!state_name||!source||!actual_environment||!actual_thread||
           !events||host.events_prefix_frame!=events||events->child!=child||events->published_events_reference<0||
           !events->published_events_table||events->master_factory_assigned!=events->factory)
            throw ScriptError("LuaEvents SAME load/preparation/Source frame changed");
        const auto& current=host.child_contexts[child];
        if(current.retired||!current.state_prepared||current.environment!=environment||current.execution_thread!=thread||
           current.source.get()!=source.get()||current.source.owner_before(source)||source.owner_before(current.source))
            throw ScriptError("LuaEvents actual child518/source/ref changed");
        const auto actual_name=current.setup_state_name_return.lock();
        if(actual_name.get()!=state_name.get()||actual_name.owner_before(state_name)||state_name.owner_before(actual_name))
            throw ScriptError("LuaEvents actual StateName return changed");
        events->first();name->require_current_without_callbacks(operation,prepared);

    }
    void full()const {
        first();events->full();first();operation.require_current();first();operation.require_same_vm(facade);first();
        operation.require_same_source(*source);first();name->require_current_owned(operation,prepared);first();
        if(!host.profile_include_collection)throw ScriptError("LuaEvents genuine include collector missing");
        host.profile_include_collection->require_current_owned(operation);first();
        host.profile_include_collection->require_current_without_callbacks(operation);first();
    }
};
