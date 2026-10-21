//Inside civ5::ui after genuine standalone InitializationLoan. This body owns
//standalone fields only; actual Root attachment Guard and active View remain
//mandatory. No plain Context/id0 metadata can enter this initializer.
void detail::NotificationLayoutStandaloneLabelState::anchor_slot4c_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    const auto& actual=InitializationLoan::Parent::node(*loan->view,loan->key);
    require(actual.parent&&*actual.parent==loan->parent_key,"Standalone79D300 actual attached38 parent changed");
    const auto* parent=InitializationLoan::Parent::node(*loan->view,loan->parent_key).state;
    require(parent&&parent->kind_==NotificationLayoutBaseline::Kind::context,
        "Standalone initial root child requires genuine after-attach Context parent");
    const auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
    const auto result=InitializationLoan::Parent::base_anchor_coordinates(*loan->baseline,parent);loan->full();
    //Actual79D309/312 first store raw margins before any anchor correction.
    loan->put_geometry_owned(0x2c,loan->geometry_owned(0x74));
    loan->put_geometry_owned(0x28,loan->geometry_owned(0x70));
    if(flags&0x70u)loan->put_geometry_owned(0x28,result[0]);
    if(flags&0x380u)loan->put_geometry_owned(0x2c,result[1]);
    loan->full();loan->view->record_four_c_return_owned(loan->key);loan->full();
}
void detail::NotificationLayoutStandaloneLabelState::initialize_common_5761_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    require(fields->init_stage==Impl::InitStage::defaults_returned&&fields->initializer_fields_returned,
        "Standalone5761 no genuine710270 prefix return");
    loan->store(0x40);std::get<std::uint32_t>(fields->fields[0x40/4])|=1u;
    loan->baseline->base_.flags_40=std::get<std::uint32_t>(fields->fields[0x40/4]);loan->first();
    anchor_slot4c_owned(loan);loan->full(); //5761EC OR1 BEFORE actual v4C79D300.
    const auto add=[](float a,float b){volatile float out=a+b;require(std::isfinite(out),"Standalone5761 FP32 overflow");return float(out);};
    loan->put_geometry_owned(0x20,float(std::fabs(static_cast<long double>(loan->geometry_owned(0x70)))+loan->geometry_owned(0x30)));
    loan->put_geometry_owned(0x24,float(std::fabs(static_cast<long double>(loan->geometry_owned(0x74)))+loan->geometry_owned(0x34)));
    loan->put_geometry_owned(0x14,loan->geometry_owned(0x2c));loan->put_geometry_owned(0x10,loan->geometry_owned(0x28));
    loan->put_geometry_owned(0x18,add(loan->geometry_owned(0x10),loan->geometry_owned(0x30)));
    loan->put_geometry_owned(0x1c,add(loan->geometry_owned(0x14),loan->geometry_owned(0x34)));
    //Common4 genuinely allocated78 even count0. This is a positive real
    //array custody check, not substitution of constructorNULL for empty.
    loan->require_actual_empty_children_owned();loan->full();
    fields->init_stage=Impl::InitStage::common_returned;loan->full();
}
void detail::NotificationLayoutStandaloneLabelState::bounds_slot94_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    loan->full();loan->require_actual_empty_children_owned();
    std::array<float,4> old{};for(unsigned i=0;i<4;++i)old[i]=loan->geometry_owned(0x10+i*4);
    const auto add=[](float a,float b){volatile float out=a+b;require(std::isfinite(out),"Standalone6EBC30 FP32 overflow");return float(out);};
    const std::array<float,4> next{loan->geometry_owned(0x28),loan->geometry_owned(0x2c),
        add(loan->geometry_owned(0x28),loan->geometry_owned(0x30)),add(loan->geometry_owned(0x2c),loan->geometry_owned(0x34))};
    for(unsigned i=0;i<4;++i)loan->put_geometry_owned(0x10+i*4,next[i]);
    if(old!=next){
        //Actual6EBD51 invokes SAME physical parent v94, no fake returned bool.
        loan->full();InitializationLoan::Parent::slot94(*loan->view,loan->parent_key);loan->full();
    }
    loan->full();
}
