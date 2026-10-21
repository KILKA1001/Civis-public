// Inside ui AFTER actual Core, RuntimeDimensionsState, SAME GUI manager implementation.
std::string detail::NotificationLayoutRuntimeDimensions::tooltip_text_from_storage_owned(
    const std::shared_ptr<State>& saved,std::string_view input){
    require_storage_without_callbacks(saved);
    require(!saved->registering&&saved->id&&saved->baseline->parser_runtime_dimensions_==saved->facade&&
        ((saved->baseline->kind_==NotificationLayoutBaseline::Kind::button&&saved->baseline->button_)||
         (saved->baseline->kind_==NotificationLayoutBaseline::Kind::image&&saved->baseline->actual_type_=="Image"&&saved->baseline->image_)),
        "Button ToolTip outside actual canonical parser occurrence");
    const std::string key(input);std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    {const auto actual=saved->core.lock();require(bool(actual),"Button ToolTip actual Core retired");source=actual->source;}
    require_storage_without_callbacks(saved);
    auto result=source->tooltip_localized_text_owned(*saved->operation,key);
    require_storage_without_callbacks(saved);
    {const auto actual=saved->core.lock();require(actual&&actual->source==source&&
        !source.owner_before(actual->source)&&!actual->source.owner_before(source),"Button ToolTip Source replaced");}
    require_storage_without_callbacks(saved);return result;
}
void detail::NotificationLayoutRuntimeDimensions::tooltip_changed_from_storage_owned(const std::shared_ptr<State>& saved){
    require_storage_without_callbacks(saved);
    require(!saved->registering&&saved->id&&saved->baseline->parser_runtime_dimensions_==saved->facade&&
        ((saved->baseline->kind_==NotificationLayoutBaseline::Kind::button&&saved->baseline->button_)||
         (saved->baseline->kind_==NotificationLayoutBaseline::Kind::image&&saved->baseline->actual_type_=="Image"&&saved->baseline->image_))&&
        saved->baseline->base_.tooltip_54&&(saved->baseline->base_.flags_40&0x4000u),
        "Button ToolTip real54/flags4000 missing before GUI524CC0");
    const auto manager=saved->manager.lock();require(bool(manager),"Button ToolTip SAME GUI retired");
    manager->refresh_tooltip_if_hovered_owned(*saved->operation,*saved->tree_facade,saved->id);
    require_storage_without_callbacks(saved);
}
