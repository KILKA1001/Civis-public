// After complete actual Source operation Impl. No copied config bool issuer.
std::shared_ptr<const BnwContextLuaProfileConfigLoan> BnwWorldViewSourceOperationLoan::context_lua_profile_config_owned()const{
    require_origin_without_callbacks();require_render_operation_owned();const auto& p=*impl_;
    try{
        const auto serial=operation_serial();std::shared_ptr<const BnwContextLuaProfileConfigLoan> result;
        {const auto source=p.owner.lock();need(bool(source),"Lua config actual Source facade retired");
         result=source->selected_lua_profile_config_owned(*this);}
        require_origin_without_callbacks();require_operation(serial);require_origin_without_callbacks();return result;
    }catch(...){p.failed.store(true);p.lease.invalidate();throw;}
}
