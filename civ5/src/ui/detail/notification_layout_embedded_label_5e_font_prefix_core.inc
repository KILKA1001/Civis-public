// Inside civ5::ui AFTER Realm, StringLoan, unchanged Child ctor and FactoryCore.
// Only actual PRESENT String -> ctor RET -> parent178 publication calls here.
void detail::NotificationLayoutRuntimeDimensions::run_published_label_5e_font_prefix_owned(
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>& returned,
    std::size_t index)const{
    const auto saved=state_;require_label_factory_return_owned(saved,returned,index);
    const auto argument=context_realm_argument_owned();
    const auto stamp=returned->impl_;
    require(stamp->published_sequence,"5E3A90 requires actual published Label178");
    const auto child=stamp->child;const auto fields=child->impl_;
    const auto active=saved->active_button_string_layer_;
    enum class Reached { constructor_return,flags_store,realm_store };
    Reached reached=Reached::constructor_return;
    const auto pure=[&]{
        // Independent child life first, then parser/source/actual178/current
        // lexical loan and genuine ordinary second argument. No parent3C read.
        fields->first();require_label_factory_return_owned(saved,returned,index);
        require_context_argument_owned(saved,argument);fields->first();
        require(active&&saved->active_button_string_layer_.get()==active.get()&&
            !active.owner_before(saved->active_button_string_layer_)&&
            !saved->active_button_string_layer_.owner_before(active),
            "5E3A90 actual String occurrence changed");
        require_button_string_layer_owned(saved,active,index);
        require(fields->entered&&!fields->busy,"5E3A90 constructor did not leave its actual call scope");
        if(reached!=Reached::constructor_return){
            const auto* flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
            require(flags&&*flags==0x0b000000u,"5E3A90 actual child flag store changed");
        }
        if(reached==Reached::realm_store){
            const auto* realm=std::get_if<NotificationLayoutEmbeddedLabelState::Impl::ParsedRealm>(
                &fields->fields[0x3c/4]);
            require(realm&&realm->value&&realm->value.get()==argument->impl_->realm.get()&&
                !realm->value.owner_before(argument->impl_->realm)&&
                !argument->impl_->realm.owner_before(realm->value),
                "5E3A90 actual child3C no longer owns the lent Realm");
        }
    };
    try{
        pure();
        // Source4 ctor body is unchanged. These are subsequent literal5E
        // stores, not a baseline copy or a boolean claiming parser completion.
        const auto* flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(flags&&*flags==0x09000000u&&
            std::holds_alternative<NotificationLayoutEmbeddedLabelState::Impl::Null>(fields->fields[0x3c/4]),
            "5E3A90 first entry requires actual constructor flags/NULL3C");
        const auto before=fields->sequence;
        require(before<=UINT64_MAX-2&&fields->history.max_size()-fields->history.size()>=2,
            "5E3A90 owned store history exhausted before writes");
        fields->history.reserve(fields->history.size()+2);pure();
        require(fields->sequence==before,"5E3A90 child changed across history allocation");
        // No allocations/callbacks occur inside these two owned store groups.
        fields->history.push_back({++fields->sequence,0x40});
        fields->fields[0x40/4]=std::uint32_t{0x0b000000u}; //5E3AC4 OR02000000
        reached=Reached::flags_store;
        pure();
        fields->history.push_back({++fields->sequence,0x3c});
        fields->fields[0x3c/4]=NotificationLayoutEmbeddedLabelState::Impl::ParsedRealm{
            argument->impl_->realm}; //5E3AD9 actual secondarg -> child3C
        reached=Reached::realm_store;
        pure();
        // Original409390 asks "Font", length4, from THIS reached XML layer.
        // Presence is distinct from empty. Copy before any Source callback;
        // raw XML/string bytes never serve as Font/Realm readiness authority.
        std::optional<std::string> font_name;
        for(const auto& entry:active->reached_layer_->attributes)if(entry.first=="Font"){
            font_name.emplace(entry.second);break;
        }
        pure();saved->operation->require_current();pure();
        if(!font_name)
            throw std::runtime_error("Label5E earned child3C; absent Font continues into unsupported later5E parser effects");
        const auto manager=saved->manager.lock();
        require(bool(manager),"Label5E Font actual canonical GUI retired");
        pure();
        auto font_return=manager->begin_font_lookup_owned(*saved->operation,
            std::optional<std::string_view>{std::string_view{*font_name}});
        pure();
        require(bool(font_return),"Label5E completed Font issuer returned unsupported NULL");
        std::shared_ptr<const BnwCompletedFontLoan> font(std::move(font_return));
        pure();
        child->enter_first_font_setter_owned(*this,returned,font,index);
        pure(); //Setter kept the literal initial5E flags/Realm; real CC is earned.
        child->parse_post_font_attributes_owned(*this,returned,index);
        //The suffix may earn new flags, so never reapply initial-prefix flag
        //equality as a stale epoch repair. Its own pure POST checks those stores.
        //The complete supported actual5E suffix returned. This grants no
        //initialized/positioned run; actual enclosing Button common parser
        //and its later6F6C80 initializer remain separate mandatory producers.
        return;
    }catch(...){
        fields->failed.store(true);fail_storage_without_callbacks(saved);throw;
        // Outer lexical catcher retires String loan FIRST, then quarantines
        // the actual Core/GUI and preserves child40/3C and reached Font stores.
    }
}
