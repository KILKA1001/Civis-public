// Outside Host::Impl, AFTER actual Preparation and StateName definitions.
namespace civ5::runtime {
std::shared_ptr<const ui::detail::NotificationContextLuaLoadSetupReturn>
LuaScriptHost::prepare_context_lua_load_setup_owned(const BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const ui::detail::NotificationContextLoadNameLoan>& name_input)const {
    const auto prepared=prepared_input;const auto name=name_input;const auto p=impl_;
    try {
        if(!prepared||!prepared->impl_||!name)
            throw ScriptError("Context preRead setup requires actual retained518/name");
        const auto saved=prepared->impl_;
        const auto core=saved->core;const auto cursor=saved->cursor;
        require_context_lua_state_owned(op,prepared,core,cursor);
        name->require_current_owned(op,prepared);
        require_context_lua_state_owned(op,prepared,core,cursor);
        const auto registration=name->register_context_resize_owned(op,prepared);
        name->require_context_resize_owned(op,prepared,registration);
        require_context_lua_state_owned(op,prepared,core,cursor);
        const auto state_name=prepare_context_lua_setup_state_name_owned(op,prepared,name);
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
        name->require_context_resize_owned(op,prepared,registration);
        require_context_lua_setup_state_name_owned(op,prepared,name,state_name);
        // Actual next call45739F ->61EDD0(profile1) ->46C420 ->4EC020.
        // This executes the protected profile body: GCSTOP, selected INI debug,
        // restricted libraries, then genuine ordered Bank IncludeCollection.
        // A missing collection throws at its real acquisition boundary.
        const auto profile=prepare_context_lua_debug_config_prefix_owned(op,prepared,name,state_name);
        require_context_lua_debug_config_prefix_owned(op,prepared,name,state_name,profile);
        name->require_context_resize_owned(op,prepared,registration);
        require_context_lua_debug_config_prefix_owned(op,prepared,name,state_name,profile);
        const auto completed=complete_context_lua_profile_owned(op,prepared,name,state_name,profile);
        require_context_lua_profile_completed_owned(op,prepared,name,state_name,completed);
        name->require_context_resize_owned(op,prepared,registration);
        require_context_lua_profile_completed_owned(op,prepared,name,state_name,completed);
        // Actual5ACF20 calls the SAME completed518 profile through60ECE0.
        // Every owner VALUE is captured before entering its protected job.
        const auto source=saved->source;const auto config=profile->impl_->config;
        const auto actual_state_name=state_name->impl_->value;
        require_context_lua_profile_completed_owned(op,prepared,name,state_name,completed);
        name->require_context_resize_owned(op,prepared,registration);
        require_context_lua_profile_completed_owned(op,prepared,name,state_name,completed);
        {
            Impl::WorldViewNativeHostScope native(*p,*this,op);p->worldview_require_callback_origin();
            native.child=saved->child;native.child_source=source;
            if(saved->child>=p->child_contexts.size())throw ScriptError("Events actual child518 retired");
            Impl::Job events{p.get(),Impl::Operation::native_context_events_factory_prefix,
                p->child_contexts[saved->child].name,{},{},{},{},{}};
            events.worldview_child=saved->child;events.context_setup_preparation=prepared;
            events.context_setup_name_loan=name;events.context_profile_state_name=state_name;
            events.context_profile_config=config;events.context_state_name=actual_state_name;
            events.context_state_thread=saved->actual_thread;events.context_state_environment=saved->actual_environment;
            Impl::NativeContextEventsPrefixFrame frame(*p,*this,op,prepared,name,state_name,completed,registration,
                source,saved->core,saved->child,saved->environment,saved->thread,saved->actual_environment,saved->actual_thread);
            frame.full();p->call(events);
            // Events5ACF20/573AC0 now completed all four actual registrars,
            // the genuine547460 lazy factory and4637C0 selected table.
            frame.full();
            if(events.context_events_reference!=frame.published_events_reference||
               events.context_events_table!=frame.published_events_table||events.context_events_reference<0||!events.context_events_table)
                throw ScriptError("Events actual selected publication data did not return");
            // Actual next4573B5 ->658430/4EEFB0/700540, with this very
            // live completed Events predecessor, SAME518/profile/Source.
            Impl::NativeContextLuaEventsFrame lua_frame(*p,*this,op,prepared,name,state_name,source,saved->core,
                saved->child,saved->environment,saved->thread,saved->actual_environment,saved->actual_thread);
            lua_frame.full();auto lua_events=events;lua_events.operation=Impl::Operation::native_context_lua_events_registration;
            p->call(lua_events);lua_frame.full();frame.full();
            const auto& actual_child=p->child_contexts[saved->child];
            if(lua_events.context_lua_events_reference<0||!lua_events.context_lua_events_table||
               lua_events.context_lua_events_reference!=actual_child.lua_events_reference||
               lua_events.context_lua_events_table!=actual_child.lua_events_pointer)
                throw ScriptError("LuaEvents658430 actual selected table/ref data did not return");
            // Actual4573BB620DF0/68AA80 master.ArtInfo VALUE copy.
            Impl::NativeContextApiRegistrationFrame api_frame(*p);api_frame.full();
            auto api=lua_events;api.operation=Impl::Operation::native_context_api_registration_prefix;
            p->call(api);api_frame.full();lua_frame.full();frame.full();
            if(api_frame.assigned_count!=1||api.context_api_art_info_reference!=api_frame.art_info_ref||
               api.context_api_art_info_type!=api_frame.art_info_type||api_frame.art_info_ref==LUA_NOREF)
                throw ScriptError("ArtInfo620DF0 original field copy did not return real data");
            // Actual5C7880 first calls protected5605C0 DB/5C1470. The
            // ctor registers consumers; it never invokes heap/SQL methods.
            Impl::NativeContextDbRegistrationFrame db_frame(*p);db_frame.full();
            auto db=api;db.operation=Impl::Operation::native_context_db_registration;
            p->call(db);db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(db_frame.stores!=1||db.context_db_master_reference!=db_frame.master_ref||db.context_db_proxy_reference!=db_frame.proxy_ref||
               db.context_db_table!=db_frame.master||db.context_db_proxy!=db_frame.proxy||db_frame.master_ref<0||db_frame.proxy_ref<0)
                throw ScriptError("DB5C7880 original selected readonly namespace did not return real data");
            // Actual5C7880 finishes GameInfo63EE70 then GameDefines606C30.
            Impl::NativeContextDatabaseNamespacesFrame database_frame(*p);database_frame.full();
            auto database=db;database.operation=Impl::Operation::native_context_database_namespaces;
            p->call(database);database_frame.full();db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(database_frame.assigned!=2)throw ScriptError("Database original three namespaces did not execute every real store");
            for(int i=0;i<2;++i){const auto& actual=database_frame.stores[i];
                if(actual.master_ref<0||actual.proxy_ref<0||database.context_database_master_references[i]!=actual.master_ref||
                   database.context_database_proxy_references[i]!=actual.proxy_ref||database.context_database_masters[i]!=actual.master||
                   database.context_database_proxies[i]!=actual.proxy)
                    throw ScriptError("Database GameInfo/GameDefines actual selected readonly namespace data return changed");
            }
            // Actual4573DA protected4F52F0/41CEC0 UIEnumsLibrary and
            // original5D7BD0 field copies into same selected518 environment.
            Impl::NativeContextUiEnumsFrame ui_enums_frame(*p);ui_enums_frame.full();
            auto ui_enums=database;ui_enums.operation=Impl::Operation::native_context_ui_enums_registration;
            p->call(ui_enums);ui_enums_frame.full();database_frame.full();db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(ui_enums_frame.master_ref<0||ui_enums.context_ui_enums_reference!=ui_enums_frame.master_ref||
               ui_enums.context_ui_enums_table!=ui_enums_frame.master||ui_enums.context_ui_enums_copied!=ui_enums_frame.copied.size())
                throw ScriptError("UIEnums original master and selected field-copy data did not return");
            // Actual4573E1 protected4CB290/6DFB60/5605C0 Network master
            // constructor4A4600 and real49B190 readonly selected namespace.
            Impl::NativeContextNetworkFrame network_frame(*p);network_frame.full();
            auto network=ui_enums;network.operation=Impl::Operation::native_context_network_registration;
            p->call(network);network_frame.full();ui_enums_frame.full();database_frame.full();db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(network_frame.stores!=1||network.context_network_master_reference!=network_frame.master_ref||
               network.context_network_proxy_reference!=network_frame.proxy_ref||network.context_network_table!=network_frame.master||
               network.context_network_proxy!=network_frame.proxy)
                throw ScriptError("Network original constructor/readonly selected namespace return absent");
            // Actual4573E8 protected624BA0/6DFB60/5605C0 Steam ctor546FA0
            // registers13 original zero-upvalue methods without invoking SDK.
            Impl::NativeContextSteamFrame steam_frame(*p);steam_frame.full();
            auto steam=network;steam.operation=Impl::Operation::native_context_steam_registration;
            p->call(steam);steam_frame.full();network_frame.full();ui_enums_frame.full();database_frame.full();db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(steam_frame.stores!=1||steam.context_steam_master_reference!=steam_frame.master_ref||
               steam.context_steam_proxy_reference!=steam_frame.proxy_ref||steam.context_steam_table!=steam_frame.master||
               steam.context_steam_proxy!=steam_frame.proxy)
                throw ScriptError("Steam original constructor/readonly selected namespace return absent");
            // Actual4573EF protected767A60/6DFB60/5605C0 Locale ctor5DD5E0
            // installs32 actual zero-upvalue consumers in the selected518.
            Impl::NativeContextLocaleFrame locale_frame(*p);locale_frame.full();
            auto locale=steam;locale.operation=Impl::Operation::native_context_locale_registration;
            p->call(locale);locale_frame.full();steam_frame.full();network_frame.full();ui_enums_frame.full();database_frame.full();db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(locale_frame.stores!=1||locale.context_locale_master_reference!=locale_frame.master_ref||
               locale.context_locale_proxy_reference!=locale_frame.proxy_ref||locale.context_locale_table!=locale_frame.master||
               locale.context_locale_proxy!=locale_frame.proxy)
                throw ScriptError("Locale original constructor/readonly selected namespace return absent");
            // Actual4573F6 real5FBB20 installs ContentManager via50FE70,
            // then protected717E10 creates/copies actual ContentManagerEnums.
            Impl::NativeContextContentFrame content_frame(*p);content_frame.full();
            auto content=locale;content.operation=Impl::Operation::native_context_content_registration;
            p->call(content);content_frame.full();locale_frame.full();steam_frame.full();network_frame.full();ui_enums_frame.full();database_frame.full();db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(content_frame.stores!=1||content.context_content_master_reference!=content_frame.master_ref||
               content.context_content_proxy_reference!=content_frame.proxy_ref||content.context_content_table!=content_frame.master||
               content.context_content_proxy!=content_frame.proxy||content_frame.enums_ref<0||
               content.context_content_enums_reference!=content_frame.enums_ref||content.context_content_enums_table!=content_frame.enums||
               content.context_content_enums_copied!=content_frame.enums_copied.size())
                throw ScriptError("ContentManager original namespace/Enums master action return absent");
            // Actual4573FD real4947F0→6DFB60/5605C0 Modding41B400
            // registers63 exact consumers after both ContentManager actions.
            Impl::NativeContextModdingFrame modding_frame(*p);modding_frame.full();
            auto modding=content;modding.operation=Impl::Operation::native_context_modding_registration;
            p->call(modding);modding_frame.full();content_frame.full();locale_frame.full();steam_frame.full();network_frame.full();ui_enums_frame.full();database_frame.full();db_frame.full();api_frame.full();lua_frame.full();frame.full();
            if(modding_frame.stores!=1||modding.context_modding_master_reference!=modding_frame.master_ref||
               modding.context_modding_proxy_reference!=modding_frame.proxy_ref||modding.context_modding_table!=modding_frame.master||
               modding.context_modding_proxy!=modding_frame.proxy)
                throw ScriptError("Modding original constructor/readonly selected namespace return absent");
            // Actual457406: real714D80, both master and lazy exposures, scoped
            // destruction and selected5D7BD0 copy. Next original registrar is
            // separate OptionsManager6704C0; no successful skip is emitted.
            Impl::NativeContextFLuaFrame flua_frame(*p);flua_frame.full();
            auto flua=modding;flua.operation=Impl::Operation::native_context_flua_registration_prefix;
            p->call(flua);flua_frame.full();modding_frame.full();frame.full();
            if(!flua_frame.returned||flua_frame.table_ref<0||!flua_frame.table||
               flua.context_flua_reference!=flua_frame.table_ref||flua.context_flua_table!=flua_frame.table||
               flua.context_flua_copied!=flua_frame.copied)
                throw ScriptError("FLua actual714D80 completed exposure/lifetime/selected copy return changed");
            // Actual45740D: OptionsManager6704C0 real constructor/cache and
            // selected49B190 readonly alias. Original methods remain lazy.
            Impl::NativeContextOptionsFrame options_frame(*p);options_frame.full();
            auto options=flua;options.operation=Impl::Operation::native_context_options_registration;
            p->call(options);options_frame.full();flua_frame.full();frame.full();
            if(options_frame.stores!=1||options_frame.master_ref<0||options_frame.proxy_ref<0||
               options.context_options_master_reference!=options_frame.master_ref||
               options.context_options_proxy_reference!=options_frame.proxy_ref||
               options.context_options_table!=options_frame.master||options.context_options_proxy!=options_frame.proxy)
                throw ScriptError("OptionsManager original constructor/readonly selected namespace return absent");
            // Actual457414 UI follows the real OptionsManager selected store.
            Impl::NativeContextUiRegistrationFrame ui_frame(*p);ui_frame.full();
            auto ui=options;ui.operation=Impl::Operation::native_context_ui_registration;
            p->call(ui);ui_frame.full();options_frame.full();frame.full();
            if(ui_frame.stores!=1||ui_frame.master_ref<0||ui_frame.proxy_ref<0||
               ui.context_ui_master_reference!=ui_frame.master_ref||ui.context_ui_proxy_reference!=ui_frame.proxy_ref||
               ui.context_ui_master_table!=ui_frame.master||ui.context_ui_proxy_table!=ui_frame.proxy)
                throw ScriptError("UI668A80 original selected namespace did not return actual master/proxy data");
            // ROOT-ONLY insertion AFTER actual UiRegistrationFrame/Job stores1 validation;
            // native_context_matchmaking_registration dispatch/include must be composed
            // AFTER Root Options and Model UI declarations, BEFORE Path/Profiler consumers.
            Impl::NativeContextMatchmakingFrame matchmaking_frame(*p);matchmaking_frame.full();
            if(!matchmaking_frame.predecessor->actual_job)throw ScriptError("Matchmaking requires actual returned UI Job");
            auto matchmaking=*matchmaking_frame.predecessor->actual_job;matchmaking.operation=Impl::Operation::native_context_matchmaking_registration;
            p->call(matchmaking);matchmaking_frame.full();frame.full();
            if(matchmaking_frame.stores!=1||matchmaking.context_matchmaking_master_reference!=matchmaking_frame.master_ref||
               matchmaking.context_matchmaking_proxy_reference!=matchmaking_frame.proxy_ref||
               matchmaking.context_matchmaking_master_table!=matchmaking_frame.master||
               matchmaking.context_matchmaking_proxy_table!=matchmaking_frame.proxy)
                throw ScriptError("Matchmaking original constructor/readonly selected namespace return absent");
            // NEXT actual457422 Path4F73C0 must execute with THIS live predecessor.
            // Root composes Physical Path/Profiler and remaining real registrars here.
            // Parent integration owns this ordered same-scope continuation after actual Matchmaking.
            Impl::NativeContextPathRegistrationFrame path_frame(*p);path_frame.full();
            auto path=matchmaking;path.operation=Impl::Operation::native_context_path_registration;
            p->call(path);path_frame.full();matchmaking_frame.full();
            if(path_frame.stores!=1||path.context_path_master_reference!=path_frame.master_ref||
               path.context_path_proxy_reference!=path_frame.proxy_ref||path.context_path_table!=path_frame.master||
               path.context_path_proxy!=path_frame.proxy||path_frame.master_ref<0||path_frame.proxy_ref<0)
                throw ScriptError("Path4F73C0 actual selected namespace return absent");
            Impl::NativeContextProfilerRegistrationFrame profiler_frame(*p);profiler_frame.full();
            auto profiler=path;profiler.operation=Impl::Operation::native_context_profiler_registration;
            p->call(profiler);profiler_frame.full();path_frame.full();matchmaking_frame.full();
            if(!profiler_frame.returned||profiler_frame.status!=0||profiler_frame.stores!=1||
               profiler_frame.fields_created!=5||profiler_frame.table_ref<0||!profiler_frame.table||
               profiler.context_profiler_reference!=profiler_frame.table_ref||profiler.context_profiler_table!=profiler_frame.table||
               profiler.context_profiler_status!=profiler_frame.status)
                throw ScriptError("Profiler4C5740 actual protected namespace return absent");
            // Root then constructs its NativeContextPreGameFrame with this exact live Profiler predecessor.
            // PreGame has no App operand and invokes no game consumer. Logic's
            // genuine APP[6F0] capture must be inserted here BEFORE PreGame when
            // composing the complete original conditional continuation.
            const auto app_capture_owner=op.context_application_state_owned();
            const auto captured_app_state=app_capture_owner->capture_current_owned();
            Impl::NativeContextPreGameFrame pregame_frame(*p);pregame_frame.full();
            auto pregame=profiler;pregame.operation=Impl::Operation::native_context_pregame_registration;
            p->call(pregame);pregame_frame.full();profiler_frame.full();path_frame.full();matchmaking_frame.full();frame.full();
            if(!pregame_frame.returned||!pregame_frame.callback_returned||pregame_frame.fields_created!=104||
               pregame_frame.protected_status!=0||pregame_frame.table_ref<0||!pregame_frame.table||
               pregame.context_pregame_reference!=pregame_frame.table_ref||pregame.context_pregame_table!=pregame_frame.table||
               pregame.context_pregame_fields!=pregame_frame.fields_created||pregame.context_pregame_status!=pregame_frame.protected_status)
                throw ScriptError("PreGame521E90 actual protected104-field namespace normal return absent");
            // Every genuine registrar frame and actual Job above remains in this
            // SAME lexical block for the eventual conditional chain and Controls.
            const auto app_after_pregame=op.context_application_state_owned();
            const bool actual_game_gate=app_after_pregame->game_context_gate_owned(captured_app_state);
            // Original457477 is unconditional after the actual APP branch.
            // The taken branch calls this while its returned GameCore frame lives.
            const auto finish_controls=[&](const Impl::NativeContextGameCoreFrame*actual_gamecore){
                using ControlsReturn=ui::detail::NotificationContextLuaControlsTableReturn;
                using SetupReturn=ui::detail::NotificationContextLuaLoadSetupReturn;
                Impl::NativeContextControlsFrame controls_frame(*p,*this,op,prepared,name,state_name,source,saved->core,
                    saved->child,saved->environment,saved->thread,saved->actual_environment,saved->actual_thread,
                    {},{},&pregame_frame,actual_gamecore);
                controls_frame.full();auto create=pregame;create.operation=Impl::Operation::native_context_controls_statement;
                p->call(create);controls_frame.full();pregame_frame.full();frame.full();
                if(create.context_state_thread!=saved->actual_thread||create.context_state_environment!=saved->actual_environment||
                   !create.context_controls_table||create.context_controls_reference<0)
                    throw ScriptError("Controls457477 real selected518 fixed-statement normal return absent");
                const auto table_body=std::make_shared<const ControlsReturn::Impl>(p,saved->core,prepared,name,state_name,this,
                    saved->child,op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
                    create.context_controls_table,create.context_controls_reference);
                controls_frame.full();const auto table=std::shared_ptr<const ControlsReturn>(new ControlsReturn(table_body));
                p->child_contexts[saved->child].controls_table_return=table;
                controls_frame.full();frame.full();name->require_context_resize_owned(op,prepared,registration);frame.full();
                const auto actual_source=p->worldview_sources.lock();
                if(!actual_source)throw ScriptError("Controls setup actual Source facade retired before return");
                const auto active_scope=actual_source->capture_active_operation_scope_serial_without_callbacks(&op);
                actual_source->require_operation_origin_without_callbacks(op,active_scope);
                const auto payload=std::make_shared<SetupReturn::Impl>(p,this,actual_source,prepared,name,registration,state_name,
                    completed,table,saved->child,active_scope,op.operation_serial(),p->failure_epoch,p->failed.writes_,p->execution_uncertainty);
                const auto result=std::shared_ptr<const SetupReturn>(new SetupReturn(payload));payload->canonical=result;
                p->child_contexts[saved->child].load_setup_completed_return=result;
                controls_frame.full();frame.full();
                require_context_lua_load_setup_current_without_callbacks(op,prepared,*result);
                native.returned=true;return result;
                // Next real457481 RootRead: policy/XML/defaults/namedControls,
                // real controls/export/loaded bytes/postread and their cleanup.
            };
            if(actual_game_gate){
                Impl::NativeContextGameEventsFrame game_events_frame(*p,app_capture_owner,captured_app_state);
                game_events_frame.full();auto game_events=pregame;
                game_events.operation=Impl::Operation::native_context_game_events_registration;
                p->call(game_events);game_events_frame.full();pregame_frame.full();frame.full();
                if(!game_events_frame.returned||!game_events_frame.callback_returned||game_events_frame.protected_status||
                   game_events_frame.table_ref<0||!game_events_frame.table||
                   game_events.context_game_events_reference!=game_events_frame.table_ref||
                   game_events.context_game_events_table!=game_events_frame.table)
                    throw ScriptError("GameEvents7AD340 actual selected proxy normal return absent");
                // Actual457469 ->639700/766CC0. Every owning supplier is
                // captured here in C++ before the protected Lua constructor.
                Impl::NativeContextGameCoreFrame gamecore_frame(*p);gamecore_frame.full();
                auto gamecore=game_events;gamecore.operation=Impl::Operation::native_context_gamecore_registration;
                p->native_gamecore_prepare_registration_owned(gamecore_frame,gamecore,config);
                p->call(gamecore);gamecore_frame.full();game_events_frame.full();pregame_frame.full();frame.full();
                if(!gamecore_frame.returned||!gamecore_frame.callback_returned||gamecore_frame.protected_status||
                   gamecore_frame.table_ref<0||!gamecore_frame.table||
                   gamecore.context_gamecore_reference!=gamecore_frame.table_ref||
                   gamecore.context_gamecore_table!=gamecore_frame.table||
                   gamecore.context_gamecore_copied!=gamecore_frame.copied)
                    throw ScriptError("GameCore639700 real scoped/exporter/selectedcopy normal return absent");
                // World17's exact common Controls tail runs while CoreFrame/Job
                // and every genuine predecessor above remain lexical live.
                return finish_controls(&gamecore_frame);
            }
            return finish_controls(nullptr); // genuine APP false path omits only GameEvents/GameCore
        }
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
