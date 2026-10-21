// Inside ui, after actual Core and RuntimeDimensions::State. No GUI/resource issuer.
std::array<float,4> detail::NotificationLayoutRuntimeDimensions::image_color_from_storage_owned(
    const std::shared_ptr<State>& saved,std::string_view input){
    require_storage_without_callbacks(saved);
    require(!saved->registering&&saved->baseline->parser_runtime_dimensions_==saved->facade&&
        ((saved->baseline->kind_==NotificationLayoutBaseline::Kind::image&&saved->baseline->image_)||
         (saved->baseline->kind_==NotificationLayoutBaseline::Kind::button&&saved->baseline->button_)),
        "Color outside actual canonical Image/Button parser occurrence");
    const std::string request(input);std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    {const auto actual=saved->core.lock();require(bool(actual),"Root Image Color actual Core retired");source=actual->source;}
    require_storage_without_callbacks(saved);
    const auto result=source->image_color_value_owned(*saved->operation,request);
    // FIRST after palette callbacks precedes every baseline/Core dereference.
    require_storage_without_callbacks(saved);
    {const auto actual=saved->core.lock();require(actual&&actual->source.get()==source.get()&&
        !actual->source.owner_before(source)&&!source.owner_before(actual->source),
        "Root Image Color actual parser Source replaced");}
    require_storage_without_callbacks(saved);return result;
}
std::array<float,4> BnwWorldViewControlTree::image_color_rgba_owned(
    const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=core_;saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
    const auto id=saved->checked(control);
    const auto pure=[&]{saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation,"Image RGBA reader SAME Source replaced");
        saved->require_layout_resources_owned();saved->layout_origin();};
    try{
        pure();const auto& node=Core::node(saved->nodes,id);
        require(node.layout_baseline&&node.layout_receiver_occurrence&&node.layout_baseline->parsed_source_serial_&&
            node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::image&&
            node.layout_baseline->actual_type_=="Image"&&node.layout_baseline->image_,
            "Image RGBA reader requires actual canonical constructor/parser Image");
        const auto image=node.layout_baseline->image_;const auto result=image->color_98_a4;
        pure();require(Core::node(saved->nodes,id).layout_baseline->image_==image,
            "Image RGBA canonical field owner replaced during read");return result;
    }catch(...){saved->failed.store(true);throw;}
}
