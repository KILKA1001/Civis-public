// Inside civ5::ui after complete RuntimeDimensions::State. The only issuer is
// the closed real Button54395B PRESENT loop below, never a public callback.
class detail::NotificationLayoutEmbeddedLabelStringLayerLoan final {
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutEmbeddedLabelState;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::weak_ptr<NotificationLayoutRuntimeDimensions::State> parser_;
    const std::weak_ptr<NotificationLayoutButtonState> parent_;
    const content::XmlNode* const reached_layer_;
    const std::size_t index_;
    const std::uint64_t serial_,topology_;
    mutable std::uint64_t baseline_sequence_;
    mutable std::weak_ptr<NotificationLayoutEmbeddedLabelState> published_child_;
    const std::uint32_t before_flags_,after_flags_;
    mutable std::atomic<bool> active_{true};
    NotificationLayoutEmbeddedLabelStringLayerLoan(
        const std::shared_ptr<NotificationLayoutRuntimeDimensions::State>& parser,
        const std::shared_ptr<NotificationLayoutButtonState>& parent,
        const content::XmlNode& layer,std::size_t index,std::uint32_t before_flags,
        std::uint32_t after_flags,std::uint64_t sequence)
        :parser_(parser),parent_(parent),reached_layer_(&layer),index_(index),
         serial_(parser->serial),topology_(parser->topology),baseline_sequence_(sequence),
         before_flags_(before_flags),after_flags_(after_flags){}
public:
    ~NotificationLayoutEmbeddedLabelStringLayerLoan()=default;
    NotificationLayoutEmbeddedLabelStringLayerLoan(const NotificationLayoutEmbeddedLabelStringLayerLoan&)=delete;
    NotificationLayoutEmbeddedLabelStringLayerLoan& operator=(const NotificationLayoutEmbeddedLabelStringLayerLoan&)=delete;
};
void detail::NotificationLayoutRuntimeDimensions::require_button_string_layer_owned(
    const std::shared_ptr<State>& saved,
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelStringLayerLoan>& loan,
    std::size_t index){
    // FIRST before dereferencing borrowed loop storage; retirement makes it
    // permanently unreadable even if a child keeps the token alive.
    require(loan&&loan->creator_==std::this_thread::get_id()&&loan->active_.load(),
        "Button String lexical occurrence retired/foreign before parser access");
    require_storage_without_callbacks(saved);
    const auto owner=loan->parser_.lock();const auto parent=loan->parent_.lock();
    require(owner&&saved&&owner.get()==saved.get()&&!owner.owner_before(saved)&&!saved.owner_before(owner)&&
        saved->active_button_string_layer_&&saved->active_button_string_layer_.get()==loan.get()&&
        !saved->active_button_string_layer_.owner_before(loan)&&!loan.owner_before(saved->active_button_string_layer_)&&
        index==loan->index_&&saved->serial==loan->serial_&&saved->topology==loan->topology_,
        "Button String is not the actual active parser occurrence");
    const auto* baseline=saved->baseline;
    require(baseline&&baseline->kind_==NotificationLayoutBaseline::Kind::button&&baseline->actual_type_=="Button"&&
        baseline->parser_runtime_dimensions_==saved->facade&&!baseline->parsed_source_serial_&&
        baseline->button_&&parent&&baseline->button_.get()==parent.get()&&
        !baseline->button_.owner_before(parent)&&!parent.owner_before(baseline->button_)&&
        baseline->sequence_==loan->baseline_sequence_&&baseline->base_.flags_40==loan->after_flags_&&
        loan->after_flags_==(loan->before_flags_|0x08000000u)&&index<baseline->parser_layers_.size(),
        "Button String actual parent/store history changed");
    const auto& retained=baseline->parser_layers_[index];const auto& reached=*loan->reached_layer_;
    require(retained.name==reached.name&&retained.attributes==reached.attributes&&
        std::any_of(reached.attributes.begin(),reached.attributes.end(),[](const auto& e){return e.first=="String";}),
        "Button String PRESENT layer changed or is absent");
    const auto published=loan->published_child_.lock();
    if(published){
        require(parent->child_178_observation&&
            *parent->child_178_observation==NotificationLayoutButtonState::Child178Observation::factory_child&&
            !parent->pending_label_178_&&parent->label_178_&&parent->label_178_.get()==published.get()&&
            !parent->label_178_.owner_before(published)&&!published.owner_before(parent->label_178_)&&
            parent->label_178_factory_return_,"Button String actual factory publication changed");
    }else{
        require(parent->child_178_observation&&
            *parent->child_178_observation==NotificationLayoutButtonState::Child178Observation::constructor_null&&
            !parent->label_178_&&!parent->label_178_factory_return_,
            "Button String prefix cannot borrow an unissued Label178");
    }
    require_storage_without_callbacks(saved);
}
