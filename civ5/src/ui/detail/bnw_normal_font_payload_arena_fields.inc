    //OWN memory port. Native Queue base24+offset becomes a closed storage
    //control+offset; no PE pointer is aliased. Bytes NOT globally zero-filled.
    struct NormalFontPayload144 final {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true};const std::uint32_t offset;
        alignas(16) std::array<std::byte,0x90> storage;
        std::array<bool,0x90/4> written{};
        bool scalar_stores=false;
        explicit NormalFontPayload144(std::uint32_t actual_offset):offset(actual_offset){}
        ~NormalFontPayload144(){alive.store(false);}
        void store_word(std::size_t offset_bytes,std::uint32_t word){
            require(offset_bytes%4==0&&offset_bytes<storage.size(),"Normal payload fixed physical word outside144 bytes");
            const auto bytes=std::bit_cast<std::array<std::byte,4>>(word);
            for(std::size_t i=0;i<4;++i)storage[offset_bytes+i]=bytes[i];
            written[offset_bytes/4]=true;
        }
    };
    //Closed historical entry type is completed after actual Payload/Binding.
    struct NormalFontDrawEntry;
    struct NormalFontPayloadQueue final {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true};bool allocating=false,appending=false;
        std::uint32_t used_40=0;std::uint16_t length_4c=0;
        std::uint64_t allocation_revision=0;
        std::shared_ptr<NormalFontPayload144> current_30;
        std::weak_ptr<BnwNormalFontPayloadPrefix::Impl> current_loan;
        std::vector<std::shared_ptr<NormalFontPayload144>> arena;
        //A fresh OWN queue has an empty entry table; these port controls
        //never claim original pool IDs or unwritten entry bytes.
        std::uint32_t entry_count_c=0;std::uint64_t append_sequence=0;
        std::uint32_t outer_index_4=0;bool outer_open_18=false;
        std::vector<std::shared_ptr<NormalGuiOuterWindow>> outer_windows;
        std::vector<std::shared_ptr<const NormalFontDrawEntry>> entries;
        ~NormalFontPayloadQueue(){alive.store(false);for(const auto& p:arena)p->alive.store(false);}
    };
