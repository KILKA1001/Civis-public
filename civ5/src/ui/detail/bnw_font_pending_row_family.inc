// Nested in the actual private Font allocation. Not a public query issuer.
struct PendingRowFamily final {
    struct Life final {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true};mutable std::atomic<bool> failed{false};
        void first()const{if(creator!=std::this_thread::get_id()||!alive.load()||failed.load()){
            failed.store(true);throw std::runtime_error("Font pending-row family retired/foreign/failed");}}
    };
    const std::shared_ptr<Life> life=std::make_shared<Life>();
    const std::uint64_t serial,before_store,resource_epoch;
    const std::size_t ordinal;
    const void* const font_allocation;
    const runtime::detail::BnwFontResourceLoan* const resource;
    const std::weak_ptr<const void> resource_life;
    const content::XmlNode* const node;
    const std::shared_ptr<const content::XmlNode> document;
    const std::string original_src;
    struct Slot {const std::weak_ptr<const void> life;const std::uint64_t occurrence;bool returned=false;};
    std::optional<Slot> base,index;
    struct DerivedIndex {const std::string filename;const std::int32_t marker;const runtime::FileIdentity actual_source;};
    std::optional<DerivedIndex> derived;
    std::shared_ptr<const graphics::detail::BnwFontTextureBorrower> expected_base,expected_active;
    bool row_stored=false;
    PendingRowFamily(std::uint64_t source_serial,std::uint64_t before,std::size_t row,
        const void* actual_font,const runtime::detail::BnwFontResourceLoan& loan,
        std::weak_ptr<const void> loan_life,std::uint64_t loan_epoch,
        const content::XmlNode& texture,std::shared_ptr<const content::XmlNode> xml,std::string src)
        :serial(source_serial),before_store(before),resource_epoch(loan_epoch),ordinal(row),
         font_allocation(actual_font),resource(&loan),resource_life(std::move(loan_life)),
         node(&texture),document(std::move(xml)),original_src(std::move(src)){}
    ~PendingRowFamily(){retire();}
    void retire()noexcept{life->alive.store(false);}
    void first()const{life->first();}
    void register_slot(bool optional,std::weak_ptr<const void> request,std::uint64_t occurrence){
        first();require(occurrence&&!row_stored,"Font family issuer replay/sealed row");
        if(optional){require(base&&base->returned&&expected_base&&derived&&derived->marker!=0&&!index,
            "Font index requires actual positive returned base and same-row metadata");
            index.emplace(Slot{std::move(request),occurrence,false});}
        else{require(!base&&!index&&!derived,"Font base family issuer replay");
            base.emplace(Slot{std::move(request),occurrence,false});}
    }
    void require_slot(bool optional,std::weak_ptr<const void> request,std::uint64_t occurrence)const{
        first();const auto& slot=optional?index:base;require(slot&&slot->occurrence==occurrence,"Font family sibling occurrence missing");
        const auto actual=slot->life.lock();const auto borrowed=request.lock();
        require(actual&&borrowed&&actual.get()==borrowed.get()&&!slot->life.owner_before(request)&&!request.owner_before(slot->life),
            "Font family actual sibling life/control substituted");
    }
    void returned(bool optional){
        first();auto& slot=optional?index:base;require(slot&&!slot->returned&&!row_stored,"Font family query-return replay");slot->returned=true;
    }
    void pending_query(bool optional)const{
        first();const auto& slot=optional?index:base;require(slot&&!slot->returned&&!row_stored,"Font family sibling query already returned/sealed");
        if(optional)require(base&&base->returned&&expected_base&&derived&&derived->marker!=0,"Font index query lacks actual base/derived marker");
    }
    void all_queries_returned()const{
        first();require(base&&base->returned,"Font family base return missing");
        if(expected_base){require(bool(derived),"Font positive base requires actual metadata result");
            if(derived->marker!=0)require(index&&index->returned,"Font family required index return missing");
            else require(!index,"Font zero marker forbids index query");}
        else require(!derived&&!index&&!expected_active,"Font NULL base cannot derive/query optional index");
    }
};
std::shared_ptr<PendingRowFamily> pending_row_family;
