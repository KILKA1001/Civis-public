// After complete actual ButtonString loan and published-child factory.
std::uint8_t detail::NotificationLayoutRuntimeDimensions::label_language_980a_owned()const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    try{
        const auto layer=saved->active_button_string_layer_;
        require(bool(layer),"Label language getter outside actual Button String lexical occurrence");
        require_button_string_layer_owned(saved,layer,layer->index_);
        require(saved->baseline->button_&&saved->baseline->button_->label_178_&&
            saved->baseline->button_->label_178_factory_return_,"Label language getter requires actual published child factory");
        const auto returned=saved->baseline->button_->label_178_factory_return_;
        require_label_factory_return_owned(saved,returned,layer->index_); //Factory/child FIRST before Gui borrow.
        std::uint8_t result;
        {const auto manager=saved->manager.lock();require(bool(manager),"Label language actual Gui facade retired");
            result=manager->label_language_980a_owned(*saved->operation);}
        require_label_factory_return_owned(saved,returned,layer->index_); //Actual child alive/failed/publication POST.
        require_button_string_layer_owned(saved,layer,layer->index_);require_storage_without_callbacks(saved);
        return result; // Data only; caller must execute actual6F3CD0/6A2EA0 branches.
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
