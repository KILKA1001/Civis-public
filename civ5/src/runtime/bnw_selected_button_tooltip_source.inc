// Existing selected Engine/TextData binds BEFORE native Context construction.
void BnwSelectedWorldViewContextOwner::bind_tooltip_localization_owned(
    const std::weak_ptr<ui::BnwEngineUiTurnOwner>& engine,const BnwStockLuaAttemptLease& lease){
    const auto saved=state_;
    try{
        const auto serial=lease.operation_serial();saved->current();saved->lease.require_operation(serial);
        require(!saved->active&&!saved->tooltip_locale_bound,"Button ToolTip selected Locale binding replay/late");
        {const auto world=lease.world_lease();const auto captured=saved->captured_world.lock();
         require(world&&captured&&world==captured&&!world.owner_before(captured)&&!captured.owner_before(world),
            "Button ToolTip Locale binding different Source world");}
        (void)ui::BnwEngineUiTurnOwner::selected_localization_language_from_owned_weak(engine);
        saved->lease.require_operation(serial);saved->current();
        {const auto world=lease.world_lease();const auto captured=saved->captured_world.lock();
         require(world&&captured&&world==captured&&!world.owner_before(captured)&&!captured.owner_before(world),
            "Button ToolTip Locale binding Source world replaced across callback");}
        require(!saved->tooltip_locale_bound&&!engine.expired(),"Button ToolTip Locale lender retired/reentered");
        saved->tooltip_locale=engine;saved->tooltip_locale_bound=true;
    }catch(...){saved->failed.store(true);throw;}
}
std::pair<std::string,std::uint8_t> BnwSelectedWorldViewContextOwner::selected_language_extended_font_owned(
    const BnwWorldViewSourceOperationLoan& operation)const{
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    const auto saved=state_;const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(saved->civis_sources&&saved->tooltip_locale_bound&&!saved->tooltip_locale.expired()&&!saved->tooltip_querying,
            "Gui980A actual Source Bank/selected Engine locale unbound/retired/reentered");
        saved->tooltip_querying=true;struct End{std::shared_ptr<State> s;~End(){s->tooltip_querying=false;}}end{saved};
        const auto engine=saved->tooltip_locale;
        auto result=ui::BnwEngineUiTurnOwner::selected_localization_extended_font_from_owned_weak(engine);
        pure();require(!engine.expired()&&!engine.owner_before(saved->tooltip_locale)&&!saved->tooltip_locale.owner_before(engine),
            "Gui980A actual Source localization lender changed");
        operation.require_current();pure();return result;
    }catch(...){saved->failed.store(true);throw;}
}
std::string BnwSelectedWorldViewContextOwner::tooltip_localized_text_owned(
    const BnwWorldViewSourceOperationLoan& operation,std::string_view input)const{
    const auto saved=state_;operation.require_origin_without_callbacks();
    const auto scope=saved->scope_serial;const std::string key(input);
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(saved->tooltip_locale_bound&&!saved->tooltip_locale.expired()&&!saved->tooltip_querying,
            "Button ToolTip genuine selected Locale unbound/retired/reentered");
        require(!key.empty()&&key.size()<=4096&&key.find('\0')==std::string::npos,
            "Button ToolTip key malformed");
        saved->tooltip_querying=true;
        struct End{std::shared_ptr<State> saved;~End(){saved->tooltip_querying=false;}}end{saved};
        const auto engine=saved->tooltip_locale;
        auto result=ui::BnwEngineUiTurnOwner::selected_localization_from_owned_weak(engine,key);
        pure();require(!engine.expired()&&!engine.owner_before(saved->tooltip_locale)&&
            !saved->tooltip_locale.owner_before(engine),"Button ToolTip actual Locale owner changed");
        require(result.find('\0')==std::string::npos,"Button ToolTip localized buffer containsNUL");
        pure();return result;
    }catch(...){saved->failed.store(true);throw;}
}
