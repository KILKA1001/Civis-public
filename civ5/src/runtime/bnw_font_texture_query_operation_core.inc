// Inside runtime after complete SourceOperation::Impl and actual sealed request.
std::unique_ptr<graphics::detail::BnwFontTextureObservation>
BnwWorldViewSourceOperationLoan::font_texture_query_owned(
    const detail::BnwFontResourceLoan& resource,
    const ui::detail::BnwFontTextureRequest& request)const{
    require_render_operation_owned();require_origin_without_callbacks();
    const auto& p=*impl_;
    try{
        const auto serial=operation_serial();
        const auto request_life=request.texture_lifetime_owned(*this,resource);
        const auto occurrence=request.texture_occurrence_owned(*this,resource);
        // Genuine operation/GUI/window/Grid are stack lenders for this same
        // NativeIssuer scope, not replaceable caller provider arguments.
        const auto* const grid=p.grid;
        const auto* const gui=p.gui;
        const auto* const window=p.window;
        const auto first=[&]{
            require_origin_without_callbacks();
            const auto life=request_life.lock();
            if(!life)throw ScriptError("Font texture issued request lifetime retired");
            life->require_current_owned(occurrence);
        };
        first();
        if(!grid||!gui||!window)throw ScriptError("Font texture actual render lenders missing");
        request.require_pending_query_owned(*this,resource);first();require_operation(serial);first();
        auto result=grid->query_font_texture_owned(*this,*gui,*window,resource,request);first();
        if(!result)throw ScriptError("Font texture actual query supplier returned no observation");
        request.require_pending_query_owned(*this,resource);first();require_operation(serial);first();
        result->require_current_owned(*this,resource,request);first();return result;
    }catch(const std::exception& error){
        log_font_query_primary_failure(error.what());
        p.failed.store(true);p.lease.invalidate();throw;
    }catch(...){
        log_font_query_primary_failure("<non-std exception>");
        p.failed.store(true);p.lease.invalidate();throw;
    }
}
