// Included inside civ5::ui, before the canonical GuiPort Impl.
// Scoped native96CC known-field port, NOT full UIManager/Font completion.
struct detail::NotificationContextFontRegistryFields final {
private:
    friend class detail::NotificationContextGuiPort;
    friend class detail::BnwFontTextureRequest;
    friend class detail::BnwCompletedFontLoan;
    struct FontObject final {
        struct Life final {
            const std::thread::id creator=std::this_thread::get_id();
            std::atomic<bool> alive{true};mutable std::atomic<bool> failed{false};std::atomic<std::uint64_t> epoch{1};
            void first(std::uint64_t expected)const{
                if(creator!=std::this_thread::get_id()||!alive.load()||failed.load()||epoch.load()!=expected){failed.store(true);throw std::runtime_error("Font allocation fence retired/foreign/changed");}
            }
            void retire()noexcept{alive.store(false);epoch.fetch_add(1);}
        };
        const std::shared_ptr<Life> life=std::make_shared<Life>();
        #include "bnw_font_pending_row_family.inc"
        ~FontObject(){life->retire();if(pending_row_family)pending_row_family->retire();}
        // Real owned allocation. Unknown preceding glyph/cache fields have
        // no read API and are not invented as zeros by this bounded port.
        std::array<std::uint32_t,5> scalar_40c08_40c18;
        std::optional<std::string> name_40c1c;
        struct Style {std::array<char,64> name;std::array<std::uint32_t,3> layers;};
        struct Texture {
            // Native semantic offsets ONLY; x64 custody is not native32 ABI.
            static constexpr std::uint32_t active_offset=64,base_offset=68,allow_offset=72,bc_offset=76;
            std::array<char,64> name;
            std::shared_ptr<const graphics::detail::BnwFontTextureBorrower> active,base;
            bool allow_color;float bc;
            std::uint64_t request_occurrence,index_occurrence,source_serial,store_sequence;
            std::size_t ordinal;
            // Native padding is unknown; no invented ready-state bytes.
        };
        struct Glyph {std::array<std::uint32_t,9> bits;};
        std::optional<std::vector<Style>> styles_40c20;
        std::optional<std::vector<Texture>> textures_40c2c;
        std::optional<std::vector<Glyph>> glyphs_40c38;
        std::optional<std::map<std::uint32_t,std::uint32_t>> glyph_lookup_40c44;
        enum class Store {scalar_zero,empty_name,empty_styles,empty_textures,empty_glyphs,empty_glyph_map};
        struct Entry {Store kind;std::uint32_t offset;};
        std::vector<Entry> constructor_history;
        struct ResourceQuote {
            const std::shared_ptr<const runtime::BnwCivisOwnedStockSourceBundle> bundle;
            assets::CivisLoadedResource loaded;
            std::string request;
        };
        static_assert(noexcept(ResourceQuote(std::declval<ResourceQuote&&>())),"Font quote commit must be nonallocating move");
        std::optional<ResourceQuote> resource_quote;
        struct LoadEntry {std::string operand;bool returned;std::uint32_t offset;};
        std::vector<LoadEntry> load_history;
        std::uint64_t texture_store_sequence=0;
        struct TextureReturn {std::uint64_t source_serial,store_sequence;std::size_t ordinal;};
        std::vector<TextureReturn> texture_returns;
        struct CompletedShape {
            const std::uint64_t source_serial,generation;
            const std::size_t texture_count,style_count,glyph_count;
        };
        std::optional<CompletedShape> completion;
        struct OptionalMetadata {
            const std::string index_key;const std::int32_t marker;const runtime::FileIdentity source_identity;
            OptionalMetadata(std::string k,std::int32_t m,runtime::FileIdentity source)
                :index_key(std::move(k)),marker(m),source_identity(std::move(source)){}
        };
        std::vector<OptionalMetadata> optional_metadata;

        std::optional<std::int32_t> spacing,texture_height,texture_width;
        void initialize_owned(){
            require(constructor_history.empty(),"Font68E590 constructor replay");
            constructor_history.reserve(10);
            for(std::size_t i=0;i<scalar_40c08_40c18.size();++i){
                scalar_40c08_40c18[i]=0;
                constructor_history.push_back({Store::scalar_zero,static_cast<std::uint32_t>(0x40c08+4*i)});
            }
            // Disengaged storage is not a completed member; each real empty
            // constructor is reached here in original order, after scalars.
            // Pointer-width/sentinel addresses are Civis-owned.
            name_40c1c.emplace();
            constructor_history.push_back({Store::empty_name,0x40c1c});
            styles_40c20.emplace();
            constructor_history.push_back({Store::empty_styles,0x40c20});
            textures_40c2c.emplace();
            constructor_history.push_back({Store::empty_textures,0x40c2c});
            glyphs_40c38.emplace();
            constructor_history.push_back({Store::empty_glyphs,0x40c38});
            glyph_lookup_40c44.emplace();
            constructor_history.push_back({Store::empty_glyph_map,0x40c44});
        }
        // No completed-load constructor, positive loan, numeric DTO setter,
        // authored snapshot conversion or public Font pointer is present.
    };
    struct Slot final {std::unique_ptr<FontObject> completed;};
    struct Life final {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true};mutable std::atomic<bool> failed{false};std::atomic<std::uint64_t> epoch{1};
        void first(std::uint64_t expected)const{
            if(creator!=std::this_thread::get_id()||!alive.load()||failed.load()||epoch.load()!=expected){failed.store(true);throw std::runtime_error("Font96CC independent fence retired/foreign/changed");}
        }
        void retire()noexcept{alive.store(false);epoch.fetch_add(1);}
    };
    const std::shared_ptr<Life> life=std::make_shared<Life>();
    std::uint64_t request_sequence=0;
public:
    ~NotificationContextFontRegistryFields(){life->retire();}
private:

    const std::thread::id creator=std::this_thread::get_id();
    bool failed=false;
    const std::uint64_t constructor_source_serial;
    std::map<std::uint32_t,Slot> map_96cc;
    // Keep actual in-flight allocations through a mandatory STOP. Original
    // failed-load destructor is earned ONLY by a real false load return.
    struct Pending final {std::uint32_t key;std::uint64_t source_serial;std::unique_ptr<FontObject> font;};
    std::vector<Pending> pending;
    enum class Store {map_constructed,null_slot_inserted,font_allocated,font_constructor_returned};
    struct Entry {Store kind;std::uint32_t key;std::uint64_t source_serial;};
    std::vector<Entry> history;
    explicit NotificationContextFontRegistryFields(std::uint64_t serial):constructor_source_serial(serial){
        require(serial!=0,"Font96CC constructor has no genuine Source occurrence");
        history.push_back({Store::map_constructed,0,serial});
    }
    void first()const{
        life->first(1);
        require(creator==std::this_thread::get_id()&&!failed,"Font96CC foreign creator/poisoned");
    }
};
