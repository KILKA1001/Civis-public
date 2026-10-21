// Include inside civ5::ui::detail after Baseline/helpers. No runtime issuer.
void NotificationLayoutBaseline::construct_grid_box_fields_owned(){
    //Common4F9470 has returned into this fresh OWN constructor first.
    //Private normalized x64 field storage; no old pool addresses/counters.
    if(kind_==Kind::grid){
        need(actual_type_=="Grid"&&!grid_,"Grid5D33C0 wrong/replayed constructor");
        grid_=std::shared_ptr<NotificationLayoutGridState>(new NotificationLayoutGridState);
    }else if(kind_==Kind::box){
        need(actual_type_=="Box"&&!box_,"Box58E740 wrong/replayed constructor");
        box_=std::shared_ptr<NotificationLayoutBoxState>(new NotificationLayoutBoxState);
    }
}
void NotificationLayoutBaseline::parse_grid_box_prefix_until_required_owner_owned(
    const std::vector<content::XmlNode>& layers){
    need(!parsed_source_serial_&&!layers.empty(),"Grid/Box real parser prefix replay/no layers");
    const auto grid=grid_;const auto box=box_; //Physical escrow survives STOP exception allocation.
    const auto present=[](const content::XmlNode& layer,std::string_view key){
        for(const auto&entry:layer.attributes)if(entry.first==key)return true;
        return false;
    };
    if(kind_==Kind::grid){
        parse_grid_421d70_owned(layers);
    }else{
        need(kind_==Kind::box&&actual_type_=="Box"&&box&&!grid,"Box6A3690 wrong actual constructor variant");
        need(!box->parser_attempted&&!box->failed&&parser_runtime_dimensions_,"Box parser once failure/reentry/borrower");
        parser_runtime_dimensions_->publish_box_constructor_owned();
        box->parser_attempted=true;
        try{for(std::size_t ordinal=0;ordinal<layers.size();++ordinal){const auto&layer=layers[ordinal];
            if(present(layer,"Style")){
                box->frontier=NotificationLayoutBoxState::Frontier::style_required;
                need(false,"Box5C23C0 PRESENT Style requires real SAME Box recursive virtual44 owner");
            }
            box->absent_style_returned=true;
            if(present(layer,"Color")){
                box->frontier=NotificationLayoutBoxState::Frontier::color_required;
                need(parser_runtime_dimensions_,"Box Color needs genuine closed ordinary Runtime parser borrower");
                parser_runtime_dimensions_->run_box_color_prefix_owned(layer,ordinal);
            }
            //HideBox then438170 remain mandatory; no skipped common parse.
            box->frontier=NotificationLayoutBoxState::Frontier::common_parser_required;
            need(parser_runtime_dimensions_,"Box common needs SAME actual ordinary Runtime borrower");
            parser_runtime_dimensions_->run_box_common_prefix_owned(layer,ordinal);
            parser_runtime_dimensions_->require_box_common_return_from_parser_owned();
        }}catch(...){box->failed=true;throw;}
    }
}
