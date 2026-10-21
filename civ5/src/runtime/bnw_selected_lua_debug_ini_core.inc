// After complete actual selected input Impl, inside runtime.
std::int32_t BnwSelectedLanguageInputOwner::selected_lua_debug_value_owned(
    const std::shared_ptr<const assets::CivisResourceBankOwner>& supplied)const{
    const auto owned=impl_;owned->live();
    const auto full=[&]{
        owned->live();
        if(!supplied||!owned->own_bank||owned->captured.kind!=BnwSelectedLanguageSourceKind::selected_ini)
            refuse(BnwSelectedLocaleError::input_unavailable,"Lua profile needs explicitly selected own INI, not process language");
        const auto& origin=*owned->own_bank;
        const auto actual=origin.bank->retain_owner();
        if(actual.get()!=supplied.get()||actual.owner_before(supplied)||supplied.owner_before(actual))
            refuse(BnwSelectedLocaleError::wrong_config,"Lua profile INI belongs to another canonical Bank");
        Impl::require_own_current(owned);owned->live();
        const auto canonical=owned->self.lock();
        if(!canonical||canonical.get()!=this||canonical->impl_.get()!=owned.get())
            refuse(BnwSelectedLocaleError::input_changed,"Lua profile selected input control retired");
    };
    try{
        full();
        // Immutable OWN input bytes, independently revalidated after parsing.
        // The proven local decimal0/1 subset corresponds to native atoi cache;
        // unknown/MISS/duplicate/nonnumeric forms are not native default0.
        std::string_view source=owned->bytes;
        if(source.starts_with("\xEF\xBB\xBF"))source.remove_prefix(3);
        for(unsigned char c:source)if((c<32&&c!='\r'&&c!='\n'&&c!='\t')||c>126)
            refuse(BnwSelectedLocaleError::invalid_language_input,"Lua debug INI outside explicit ASCII subset");
        const auto trim=[](std::string_view s){
            while(!s.empty()&&(s.front()==' '||s.front()=='\t'))s.remove_prefix(1);
            while(!s.empty()&&(s.back()==' '||s.back()=='\t'))s.remove_suffix(1);return s;
        };
        const auto equal=[](const std::string& a,const char* b){return _stricmp(a.c_str(),b)==0;};
        std::vector<std::string> sections,keys;std::string section;std::optional<std::int32_t> value;
        while(!source.empty()){
            const auto end=source.find('\n');auto line=source.substr(0,end);
            source=end==source.npos?std::string_view{}:source.substr(end+1);
            if(!line.empty()&&line.back()=='\r')line.remove_suffix(1);
            line=trim(line);
            if(line.find('\r')!=line.npos||line.size()>=1024)
                refuse(BnwSelectedLocaleError::invalid_language_input,"Lua debug INI line outside bounded grammar");
            if(line.empty()||line.front()==';')continue;
            if(line.front()=='['){
                if(line.size()<3||line.back()!=']'||line.find(']',1)!=line.size()-1||line.find('[',1)!=line.npos)
                    refuse(BnwSelectedLocaleError::invalid_language_input,"Lua debug malformed section");
                section=std::string(line.substr(1,line.size()-2));
                for(const auto& old:sections)if(equal(old,section.c_str()))
                    refuse(BnwSelectedLocaleError::invalid_language_input,"Lua debug duplicate section unsupported");
                sections.push_back(section);keys.clear();continue;
            }
            const auto sep=line.find('=');if(sep==line.npos||section.empty())continue;
            const std::string key(trim(line.substr(0,sep)));
            for(const auto& old:keys)if(equal(old,key.c_str()))
                refuse(BnwSelectedLocaleError::invalid_language_input,"Lua debug duplicate key unsupported");
            keys.push_back(key);
            if(!equal(section,"Debugging")||!equal(key,"EnableLuaDebugLibrary"))continue;
            const auto numeric=trim(line.substr(sep+1));
            if(numeric!="0"&&numeric!="1")
                refuse(BnwSelectedLocaleError::invalid_language_input,"Lua debug numeric operand outside proven explicit0/1 subset");
            value=numeric=="1"?1:0;
        }
        full();
        if(!value)refuse(BnwSelectedLocaleError::input_unavailable,"Lua debug required INI operand missing");
        const auto result=*value;full();return result;
    }catch(...){owned->retired.store(true);throw;}
}
