// After complete Source State, before Child descriptors/publication.
void BnwSelectedWorldViewContextOwner::State::capture_client_context_profile_owned(
    const assets::ResourceMountTable& mounts,const BnwStockLuaAttemptLease& operation,std::uint64_t serial){
    operation.require_operation(serial);
    require(!active&&canonical_owner.expired()&&sources.size()==profile.size(),
        "Selected client Source profile capture outside real cold owner");
    // The legacy non-Civis capture keeps its fixed descriptors. Complete
    // selected-client construction is supplied only by the real own Bank.
    if(!civis_sources)return;
    std::set<std::string> declarations;std::string captured_parent="InGame";
    const auto walk=[&](auto&& self,const content::XmlNode& node)->void{
        if(node.name=="Instance"||node.name=="InstanceData")return;
        if(node.name=="LuaContext"){
            const auto stem=node.attribute("FileName");auto id=node.attribute("ID");
            if(id.empty()){const auto slash=stem.find_last_of("/\\");id=stem.substr(slash==std::string::npos?0:slash+1);}
            require(!stem.empty()&&!id.empty()&&declarations.insert(id).second&&declarations.size()<=4096,
                "Selected client Source declaration missing/ambiguous/exhausted");
            const auto route=profile_parents.find(id);
            require(route==profile_parents.end()||route->second==captured_parent,
                "Selected client reused Context ID requires genuine scoped source owner");
            profile_parents.insert_or_assign(id,captured_parent);
            for(std::size_t existing=0;existing<profile.size();++existing)if(profile[existing].context==id){
                require(profile[existing].stem==stem&&loader_requests.size()==profile.size(),
                    "Selected client existing actual descriptor stem/custody differs");
                // This is the real selected declaration visit, not metadata
                // reconstructed later from a normalized key or root Name.
                const auto held_declaration=std::make_shared<const content::XmlNode>(node);
                operation.require_operation(serial);
                auto held_request=std::shared_ptr<const detail::ContextLoaderRequestCustody>(
                    new detail::ContextLoaderRequestCustody(id,stem,held_declaration));
                operation.require_operation(serial);
                loader_requests[existing]=std::move(held_request);
                operation.require_operation(serial);return;
            }
            // Preserve this exact declaration and FileName BEFORE normalization.
            const auto original_declaration=std::make_shared<const content::XmlNode>(node);
            operation.require_operation(serial);
            auto original_request=std::shared_ptr<const detail::ContextLoaderRequestCustody>(
                new detail::ContextLoaderRequestCustody(id,stem,original_declaration));
            operation.require_operation(serial);
            const auto lua=mounts.resolve(assets::AssetKey(stem+".lua"));operation.require_operation(serial);
            const auto layout=mounts.resolve(assets::AssetKey(stem+".xml"));operation.require_operation(serial);
            const auto* lh=std::get_if<assets::AssetHandle>(&lua.asset);const auto* xh=std::get_if<assets::AssetHandle>(&layout.asset);
            require(lh&&xh,"Selected client actual VFS child source missing or ambiguous");
            Source captured(*lh,*xh);operation.require_operation(serial);
            require(captured.layout.name=="Context","Selected client actual child XML is not Context");
            const auto ls=identify_bytes(captured.lua_bytes).sha256,xs=identify_bytes(captured.xml_bytes).sha256;
            // Actual VFS-derived immutable payload lineage, never caller hash
            // authority. Own Bank must hold the same selected logical winner.
            require(civis_sources->loaded(stem+".lua").payload_identity.sha256==ls&&
                civis_sources->loaded(stem+".xml").payload_identity.sha256==xs,
                "Selected client mounted source differs from own Bank winner");
            operation.require_operation(serial);
            // Logical FileName is not a physical winner path: Expansion2
            // overrides may be selected for an Assets/UI request. Retain the
            // actual confined loose winner provenance for physical rechecks.
            const auto physical=[&](const assets::AssetHandle& h)->std::string{
                const auto& provenance=h.candidate().provenance;
                require(provenance.kind==assets::SourceKind::loose&&!provenance.archive_relative_path,
                    "Selected client Context physical source requires genuine loose winner");
                const auto actual=std::filesystem::canonical(std::filesystem::u8path(provenance.source_id)/
                    std::filesystem::u8path(provenance.source_relative_path));
                const auto qualified=assets::path_utf8(actual.lexically_relative(std::filesystem::canonical(root)));
                require(!qualified.empty()&&assets::AssetKey(qualified).safe_relative_path(),
                    "Selected client actual physical winner escaped canonical assets");
                return qualified;
            };
            const auto lp=physical(*lh),xp=physical(*xh);operation.require_operation(serial);
            require(source_identity(root/lp).sha256==ls&&source_identity(root/xp).sha256==xs,
                "Selected client actual physical winner bytes differ");
            operation.require_operation(serial);
            profile.push_back({id,stem,lp,xp,ls,xs});sources.push_back(std::move(captured));
            loader_requests.push_back(std::move(original_request));operation.require_operation(serial);return;
        }
        for(const auto& child:node.children)self(self,child);
    };
    walk(walk,declaration);operation.require_operation(serial);
    // Capture the first real nested subtree recursively. Traversal here owns
    // source payloads only: constructor/attachment order is the Tree's DFS.
    // Other Root subtrees require their scoped source joins independently.
    std::vector<std::string> pending{"WorldView"};std::set<std::string> visited;
    for(std::size_t cursor=0;cursor<pending.size();++cursor){
        captured_parent=pending[cursor];
        require(visited.insert(captured_parent).second&&visited.size()<=4096,
            "Selected nested source cycle/duplicate namespace exhausted");
        declarations.clear();const auto actual_parent=sources.at(index(captured_parent)).layout;
        walk(walk,actual_parent);operation.require_operation(serial);
        const auto descend=[&](auto&& self,const content::XmlNode& node)->void{
            if(node.name=="Instance"||node.name=="InstanceData")return;
            if(node.name=="LuaContext"){
                const auto stem=node.attribute("FileName");auto id=node.attribute("ID");
                if(id.empty()){const auto slash=stem.find_last_of("/\\");id=stem.substr(slash==std::string::npos?0:slash+1);}
                pending.push_back(id);return;
            }
            for(const auto& child:node.children)self(self,child);
        };
        descend(descend,actual_parent);
    }
    operation.require_operation(serial);
}
