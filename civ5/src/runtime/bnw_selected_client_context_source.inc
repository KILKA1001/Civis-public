// In runtime after complete Source State. Normalized selected Civis client
// command; no retail CB8CF4 pointer or GameUI/App C0 is imported as authority.
void BnwSelectedWorldViewContextOwner::publish_selected_client_context_slot1_owned(
    const BnwWorldViewSourceOperationLoan& operation,
    const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>& existing){
    const auto root=existing[0];
    const auto saved=state_;operation.require_origin_without_callbacks();
    const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(static_cast<bool>(root),"Selected client root construction result absent");
        const auto metadata=root->metadata_source_owner();pure();
        require(metadata.get()==this&&!metadata.owner_before(saved->canonical_owner)&&
            !saved->canonical_owner.owner_before(metadata)&&root->context()=="InGame",
            "Selected client actual root construction has another Source");pure();
        root->construct_selected_client_context_registry_owned(operation,existing);pure();
        root->require_selected_client_context_registry_owned(operation);pure();
        root->construct_selected_nested_contexts_owned(operation,existing);pure();
        if(!saved->client_context_publications.empty()){
            const auto old=saved->client_context_slot1.lock();pure();
            require(old&&old.get()==root.get()&&!old.owner_before(root)&&!root.owner_before(old),
                "Selected client slot1 actual root was replaced");return;
        }
        // Reserve before the real slot publication. History is private command
        // output; an external number/Context name cannot earn this association.
        saved->client_context_publications.reserve(1);pure();
        saved->client_context_slot1=root;
        saved->client_context_publications.push_back({operation.operation_serial(),root});
        pure();
    }catch(...){saved->failed.store(true);throw;}
}
std::shared_ptr<ui::BnwWorldViewControlTree>
BnwSelectedWorldViewContextOwner::selected_client_context_slot1_owned(
    const BnwWorldViewSourceOperationLoan& operation)const {
    const auto saved=state_;operation.require_origin_without_callbacks();const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(saved->client_context_publications.size()==1,
            "Selected client slot1 has no genuine construction/publication command");
        const auto root=saved->client_context_slot1.lock();pure();
        require(static_cast<bool>(root),"Selected client slot1 real root facade retired");
        const auto identity=saved->client_context_publications.front().root;
        require(!identity.owner_before(root)&&!root.owner_before(identity),"Selected client slot1 command allocation differs");
        root->require_selected_client_context_registry_owned(operation);pure();
        operation.require_current();pure();saved->current();pure();
        {const auto after=saved->client_context_slot1.lock();
         require(after&&after.get()==root.get()&&!after.owner_before(root)&&!root.owner_before(after),
            "Selected client slot1 replaced across source callbacks");}
        pure();return root; // allocated root only; namespace/full Lua still gated
    }catch(...){saved->failed.store(true);throw;}
}
std::vector<std::shared_ptr<ui::BnwWorldViewControlTree>>
BnwSelectedWorldViewContextOwner::selected_client_retained_context_trees_owned(
    const BnwWorldViewSourceOperationLoan& operation)const{
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    const auto saved=state_;const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();const auto root=selected_client_context_slot1_owned(operation);pure();
        auto result=root->collect_selected_client_retained_context_trees_owned(operation,root);pure();
        const auto current=selected_client_context_slot1_owned(operation);pure();
        require(current.get()==root.get()&&!current.owner_before(root)&&!root.owner_before(current)&&
            !result.empty()&&result.front().get()==root.get(),"Retained Context enumeration canonical client root changed");
        pure();return result;
    }catch(...){saved->failed.store(true);throw;}
}
std::array<assets::CivisLoadedResource,3>
BnwSelectedWorldViewContextOwner::selected_style_resources_owned(const BnwWorldViewSourceOperationLoan& operation)const{
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    const auto saved=state_;const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(bool(saved->civis_sources),"Style initial load requires canonical staged Resource Bank");
        constexpr std::array<std::string_view,3> keys{"Assets/UI/Styles.xml",
            "Assets/DLC/Expansion/UI/Styles_Expansion1.xml","Assets/DLC/Expansion2/UI/Styles_Expansion2.xml"};
        std::array<assets::CivisLoadedResource,3> result;
        for(std::size_t i=0;i<keys.size();++i){
            pure();result[i]=saved->civis_sources->loaded(keys[i]);pure();
            require(result[i].bytes&&result[i].xml&&result[i].xml->name=="StyleSheet",
                "Style initial load missing staged immutable sheet");
        }
        operation.require_current();pure();return result;
    }catch(...){saved->failed.store(true);throw;}
}
std::map<std::string,content::XmlNode,std::less<>>
BnwSelectedWorldViewContextOwner::selected_client_styles_owned(const BnwWorldViewSourceOperationLoan& operation)const{
    operation.require_origin_without_callbacks();const auto saved=state_;const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    pure();require(static_cast<bool>(saved->civis_sources),"Selected client parser needs genuine own Bank styles");
    const auto sheets=selected_style_resources_owned(operation);pure();
    std::map<std::string,content::XmlNode,std::less<>> result;
    for(const auto& sheet:sheets){
        require(sheet.xml&&sheet.xml->name=="StyleSheet","Selected client styles root differs");
        std::map<std::string,content::XmlNode,std::less<>> first_in_sheet;
        //835A80 returns the first exact child. Later enabled providers override earlier ones.
        for(const auto& value:sheet.xml->children){require(!value.name.empty(),"Selected style has empty name");first_in_sheet.emplace(value.name,value);}
        for(const auto& value:first_in_sheet)result.insert_or_assign(value.first,value.second);
    }
    operation.require_current();pure();return result;
}
