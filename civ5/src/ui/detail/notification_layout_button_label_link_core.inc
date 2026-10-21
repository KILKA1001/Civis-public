// After complete active String loan AND genuine NewGame owned child body.
void detail::NotificationLayoutRuntimeDimensions::retain_pending_label_178_prefix_owned(
    const std::shared_ptr<State>& saved,
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState>& child,std::size_t index){
    require_storage_without_callbacks(saved);
    const auto active=saved->active_button_string_layer_;
    require_button_string_layer_owned(saved,active,index);
    const auto parent=saved->baseline->button_;
    require(child&&!parent->pending_label_178_&&!parent->label_178_,
        "Button pending Label allocation replay or missing actual child");
    child->require_ownership_origin_owned(parent,*saved->operation,index);
    require_button_string_layer_owned(saved,active,index);
    parent->pending_label_178_=child; //own BEFORE constructor callback/module work
    require_pending_label_178_prefix_owned(saved,child.get(),index);
}
void detail::NotificationLayoutRuntimeDimensions::require_pending_label_178_prefix_owned(
    const std::shared_ptr<State>& saved,const NotificationLayoutEmbeddedLabelState* child,std::size_t index){
    require_storage_without_callbacks(saved);
    const auto active=saved->active_button_string_layer_;
    require_button_string_layer_owned(saved,active,index);
    const auto parent=saved->baseline->button_;
    const auto retained=parent->pending_label_178_;
    require(child&&retained&&retained.get()==child&&!parent->label_178_,
        "Button pending Label actual allocation membership changed");
    retained->require_ownership_origin_owned(parent,*saved->operation,index);
    require_button_string_layer_owned(saved,active,index);
}
void detail::NotificationLayoutRuntimeDimensions::run_button_string_failed_prefix_owned(
    const content::XmlNode& reached,std::size_t index)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    require(!saved->active_button_string_layer_,"Button String lexical parser reentry");
    const auto* baseline=saved->baseline;
    require(baseline&&baseline->kind_==NotificationLayoutBaseline::Kind::button&&baseline->actual_type_=="Button"&&
        baseline->parser_runtime_dimensions_==this&&!baseline->parsed_source_serial_&&baseline->button_&&
        index<baseline->parser_layers_.size(),"Button String lacks actual reached canonical parser");
    const auto& retained=baseline->parser_layers_[index];
    require(retained.name==reached.name&&retained.attributes==reached.attributes&&
        std::any_of(reached.attributes.begin(),reached.attributes.end(),[](const auto& e){return e.first=="String";}),
        "Button String direct actual lookup is ABSENT or foreign");
    // Reacquire the mutable canonical allocation, not a casted snapshot. Drop
    // the checker Core lock before any child callback/module work.
    NotificationLayoutBaseline* actual_baseline=nullptr;
    {const auto actual=saved->core.lock();
        require(actual&&saved->id&&saved->id<=actual->nodes.size()&&
            actual->nodes[saved->id-1].layout_baseline,
            "Button String actual mutable node allocation absent");
        actual_baseline=&*actual->nodes[saved->id-1].layout_baseline;
        require(actual_baseline==baseline,"Button String mutable canonical allocation changed");}
    const auto before=actual_baseline->base_.flags_40;
    actual_baseline->base_.flags_40|=0x08000000u; //543968 BEFORE allocation/callbacks
    actual_baseline->record_owned(NotificationLayoutBaseline::Mutation::parser);
    std::shared_ptr<const NotificationLayoutEmbeddedLabelStringLayerLoan> active;
    try{
        active=std::shared_ptr<const NotificationLayoutEmbeddedLabelStringLayerLoan>(
            new NotificationLayoutEmbeddedLabelStringLayerLoan(saved,baseline->button_,reached,index,
                before,actual_baseline->base_.flags_40,actual_baseline->sequence_));
        saved->active_button_string_layer_=active;
        require_button_string_layer_owned(saved,active,index);
        // Closed lexical call. This is not a caller-supplied callback/token
        // factory. The real child owns its field/module effects, then stops.
        const auto returned=construct_embedded_label_factory_owned(index);
        require_label_factory_return_owned(saved,returned,index);
        publish_embedded_label_factory_owned(returned,index);
        require_label_factory_return_owned(saved,returned,index);
        // Publication54397D precedes actual child5E; its real ordinary
        // second argument is independent of the later Button common3C store.
        run_published_label_5e_font_prefix_owned(returned,index);
        saved->operation->require_current();require_label_factory_return_owned(saved,returned,index);
        active->active_.store(false); //normal lexical retirement FIRST
        saved->active_button_string_layer_.reset();
        return; //actual5E returned; common parent3C/slot10 not yet executed.
    }catch(...){
        if(active)active->active_.store(false); //FIRST before borrowed layer expires
        saved->active_button_string_layer_.reset();
        fail_storage_without_callbacks(saved); //ordinary parent/Core/GUI authority dies FIRST
        retire_failed_label_owned(saved); //retiring access preserves earned stores/unknown quarantine
        throw;
    }
}
