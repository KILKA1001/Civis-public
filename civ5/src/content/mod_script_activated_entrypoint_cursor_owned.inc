// Inside content namespace, using the SAME existing registry_ and entries_sql.
// No second registry, eager Step, gameplay DB or count-derived empty iterator.
class ModEntryPointScriptCursor final:public runtime::ScriptQuery {
    std::unique_ptr<RulesScriptCursor> cursor_;
public:
    explicit ModEntryPointScriptCursor(std::unique_ptr<RulesScriptCursor> cursor):cursor_(std::move(cursor)){}
    std::optional<runtime::ScriptRow> next()override{return cursor_->next();}
    bool step()override{return cursor_->step();}
    runtime::ScriptRow row()const override{return cursor_->row();}
    std::pair<bool,runtime::ScriptValue> cache_key(bool id)const override{return cursor_->cache_key(id);}
    void reset()override{cursor_->reset();}
    void bind(std::span<const runtime::ScriptValue>args)override{cursor_->bind(args,true);}
    void release()override{cursor_->release();}
    std::string_view error()const override{return cursor_->error();}
};
std::unique_ptr<runtime::ScriptQuery> ModScriptRegistry::activated_entrypoint_query_owned(std::string_view type)const{
    const std::array<DatabaseScriptValue,1> parameter{std::string(type.substr(0,type.find('\0')))};
    auto cursor=registry_.script_query(entries_sql,{}); // prepare; empty span makes no SQLite bind
    if(cursor)cursor->bind(parameter,true); // Original6D9946 ignores Bind return; copy=true
    return cursor?std::make_unique<ModEntryPointScriptCursor>(std::move(cursor)):nullptr;
}
