//Inside runtime. Strict lender for the consumer's actual live514 ASCII query.
//No prepared_source filename, XML parse, new Bank/VM or lazy file-read grant.
std::shared_ptr<const assets::detail::CivisLoadedBytesPhysicalPathLoan>
BnwWorldViewSourceOperationLoan::context_lua_physical_source_owned(const assets::AssetKey& key)const{
    require_render_operation_owned();require_origin_without_callbacks();
    const auto requested=key.original_name();require_origin_without_callbacks();
    const auto normalized=key.normalized_name();require_origin_without_callbacks();
    need(key.safe_relative_path()&&!requested.empty()&&normalized.ends_with(".lua")&&
        std::all_of(requested.begin(),requested.end(),[](unsigned char ch){return ch>0&&ch<128;}),
        "Lua514 resource conversion requires actual selected relative ASCII .lua query");
    const assets::AssetKey owned(requested);require_origin_without_callbacks();
    const auto first=[&]{require_origin_without_callbacks();
        need(key.original_name()==requested&&key.normalized_name()==normalized&&key.safe_relative_path()&&
            owned.original_name()==requested&&owned.normalized_name()==normalized,
            "Lua514 borrowed query changed across Source boundary");};
    first();const auto bank=selected_provider_startup_bank_owned();first();
    const auto capability=bank->capability();first();
    const auto physical=capability.loaded_bytes_physical_path_owned(owned);first();
    physical->require_same_bank_without_callbacks(*bank);first();require_current();first();
    physical->require_same_bank_without_callbacks(*bank);first();return physical;
}
