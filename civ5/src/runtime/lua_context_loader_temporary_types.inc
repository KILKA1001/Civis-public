// Inside Impl, before ChildContext. All destructors here only free owned
// C++ storage; no suppressed UI/gameplay callbacks or Original ABI claim.
struct ContextLoaderPathTemporary final {
    enum class Role {script_compile_path,context_layout_source_path,script_source_directory_path};
    const Role role;
    const std::string resolved_name,source_directory;
    const FileIdentity fingerprint;
    std::unique_ptr<assets::AssetHandle> handle;
    std::unique_ptr<assets::Bytes> buffer;
    ContextLoaderPathTemporary(Role r,std::string name,FileIdentity identity,
        std::unique_ptr<assets::AssetHandle> h,std::unique_ptr<assets::Bytes> b)
      :role(r),resolved_name(std::move(name)),source_directory(assets::path_utf8(std::filesystem::path(resolved_name).parent_path())),
       fingerprint(std::move(identity)),handle(std::move(h)),buffer(std::move(b)){}
};
struct ContextLoaderTemporaries final {
    enum class Step {script_path_destroyed,receiver_unref,environment_unref,layout_path_destroyed,directory_path_destroyed,filename_destroyed};
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> core;
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> preparation;
    const std::shared_ptr<const ScriptUiChildSource> source;
    const std::shared_ptr<const BnwCivisOwnedStockSourceBundle> bundle;
    const std::uint64_t serial,generation,epoch,writes,uncertainty;
    const std::string context,lua_path,xml_path,stem;
    std::array<std::unique_ptr<ContextLoaderPathTemporary>,3> paths;
    std::unique_ptr<std::string> filename;
    int environment=LUA_NOREF,receiver=LUA_NOREF;
    const void* actual_environment=nullptr;const void* actual_receiver=nullptr;
    lua_State* registry_state=nullptr;
    bool refs_returned=false,cleanup_started=false,cleanup_returned=false;
    std::array<Step,6> journal{};std::size_t journal_size=0;
    ContextLoaderTemporaries(std::weak_ptr<const void> c,std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation> p,
        std::shared_ptr<const ScriptUiChildSource> s,std::shared_ptr<const BnwCivisOwnedStockSourceBundle> b,
        std::uint64_t op,std::uint64_t g,std::uint64_t e,std::uint64_t w,std::uint64_t u,
        std::string n,std::string l,std::string x,std::string base)
      :core(std::move(c)),preparation(std::move(p)),source(std::move(s)),bundle(std::move(b)),serial(op),generation(g),
       epoch(e),writes(w),uncertainty(u),context(std::move(n)),lua_path(std::move(l)),xml_path(std::move(x)),stem(std::move(base)){}
    void record(Step step) noexcept {journal[journal_size++]=step;}
};
