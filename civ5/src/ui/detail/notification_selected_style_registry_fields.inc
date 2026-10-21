// Nested in private actual GuiPort::Impl. These are portable normalized
// initial-load owners, not x86 startup receipts or dynamic style registration.
struct SelectedStyleRegistry final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::array<assets::CivisLoadedResource,3> sheets;
    const std::array<std::map<std::string,content::XmlNode,std::less<>>,3> providers;
    const std::uint64_t source_serial;
    static auto build(const std::array<assets::CivisLoadedResource,3>& input){
        std::array<std::map<std::string,content::XmlNode,std::less<>>,3> result;
        for(std::size_t i=0;i<input.size();++i){
            require(input[i].bytes&&input[i].xml&&input[i].xml->name=="StyleSheet","Style provider has no actual staged sheet");
            for(const auto& row:input[i].xml->children){require(!row.name.empty(),"Style provider empty XML child name");
                result[i].emplace(row.name,row);}
        }
        return result;
    }
    explicit SelectedStyleRegistry(std::array<assets::CivisLoadedResource,3> input,std::uint64_t serial)
        :sheets(std::move(input)),providers(build(sheets)),source_serial(serial){require(serial!=0,"Style setup has no actual Source occurrence");}
    void first()const{require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"Style owner retired/foreign/failed FIRST");}
    ~SelectedStyleRegistry(){alive.store(false);}
    std::optional<content::XmlNode> lookup_global(std::string_view name)const{
        first();require(name.find('\0')==name.npos,"Style exact XML child key contains NUL");
        // Selected initial load enables Exp1/Exp2 providers in this order.
        //54A2D0 walks active providers backwards. Base is579650 Gui fallback.
        for(std::size_t i=providers.size();i>1;--i){const auto p=providers[i-1].find(name);
            if(p!=providers[i-1].end())return p->second;}
        const auto p=providers[0].find(name);if(p!=providers[0].end())return p->second;
        return std::nullopt;
    }
};
std::shared_ptr<SelectedStyleRegistry> selected_styles;
