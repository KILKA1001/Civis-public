// Inside civ5::ui, after the complete private canonical Core definition.
struct detail::NotificationLayoutContextConstructionReturn::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::uint64_t topology,effects,constructor_source_serial;
    // Snapshot cloned from the actual executed constructor, not reconstructed
    // from XML or a numeric submission. History contains its real construct.
    const NotificationLayoutBaseline constructor_state;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual,
        std::uint64_t serial)
        :core(actual),source(actual->source),topology(actual->topology_revision),effects(actual->effect_revision),
         constructor_source_serial(serial),
         constructor_state(NotificationLayoutBaseline::clone_owned(*actual->context_layout_baseline)) {
        actual->layout_origin();
        require(actual->source->current_operation_loan().operation_serial()==constructor_source_serial&&constructor_source_serial!=0,
            "Context constructor SAME entered Source operation absent");
        require(constructor_state.kind_==NotificationLayoutBaseline::Kind::context&&
            constructor_state.actual_type_=="Context"&&!constructor_state.parsed_source_serial_&&
            constructor_state.sequence_==1&&constructor_state.history_.size()==1&&
            constructor_state.history_.front().command==NotificationLayoutBaseline::Mutation::construct&&
            constructor_state.history_.front().sequence==constructor_state.sequence_,
            "Context constructor actual first native-store history differs");
        // The actual fresh receiver is still before parser/factory/attachment.
        // Its constructor values are NOT a claim of live root NULL or geometry.
        require(actual->nodes.empty()&&!actual->layout_styles&&!actual->context_parent_kind&&
            !actual->context_layout_children&&!actual->layout_context_initialization_origin&&
            !actual->layout_initialization_return&&!actual->context_initialization_active,
            "Context constructor receipt issued after parser/attachment/load");
        actual->layout_origin();
    }
};

std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn>
BnwWorldViewControlTree::capture_context_constructor_owned(const std::shared_ptr<Core>& actual,
    const runtime::BnwWorldViewSourceOperationLoan& operation) {
    using Return=detail::NotificationLayoutContextConstructionReturn;
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
    require(!actual->context_construction_return&&actual->context_layout_baseline,
        "Context constructor canonical receipt missing state or replayed");
    const auto body=std::make_shared<const Return::Impl>(actual,operation.operation_serial());
    actual->source->require_sources_current();actual->layout_origin();
    operation.require_operation(body->constructor_source_serial);actual->layout_origin();
    require(&actual->source->current_operation_loan()==&operation&&
        actual->topology_revision==body->topology&&actual->effect_revision==body->effects&&
        !actual->context_construction_return&&actual->nodes.empty(),
        "Context constructor actual receiver changed across Source callback");
    const auto result=std::shared_ptr<const Return>(new Return(body));
    actual->layout_origin();return result;
}
