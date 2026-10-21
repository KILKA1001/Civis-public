//Actual owned physical fields; these sealed objects cannot be constructed by callers.
struct detail::NotificationLayoutOrdinaryReceiver::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::weak_ptr<NotificationLayoutImageState> image;
    const std::weak_ptr<NotificationLayoutGridState> grid;
    const std::weak_ptr<NotificationLayoutBoxState> box;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> context;
    const std::weak_ptr<NotificationLayoutStandaloneLabelState> standalone_label;
    const std::weak_ptr<NotificationLayoutButtonState> button;
    //Actual embedded Stack object: weak SAMECore alias prevents ownership cycle.
    const std::weak_ptr<const void> stack;
    enum class StackParserProgress {constructor_returned,style_missing_returned,prefix_returned,common_entered,id_lookup_returned,common_returned};
    std::optional<StackParserProgress> stack_parser_progress;
    //Only the actual mandatory49A030 reader advances this private execution guard.
    enum class StackPaddingReaderProgress {unentered,missing_returned,present_extracting,present_returned};
    std::optional<StackPaddingReaderProgress> stack_padding_reader_progress;
    //A completed PRESENT49A030 owns the real payload and the SAME stream
    //locale/facets which actually extracted into this physical Stack. It is
    //an execution association, not a type/factory/readiness receipt.
    struct StackPaddingReadReturn final {
        const std::thread::id creator=std::this_thread::get_id();
        const std::weak_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
        const std::weak_ptr<const void> physical_stack;
        const std::uint64_t source_serial;
        const std::uint32_t axis84,destination_offset,before_word,after_word;
        const std::string payload;
        const std::locale actual_locale;
        const std::num_get<char>*const num_get;
        const std::ctype<char>*const ctype;
        const std::numpunct<char>*const numpunct;
        StackPaddingReadReturn(
            std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> x,
            std::shared_ptr<const void> actual,std::uint64_t serial,
            std::uint32_t axis,std::uint32_t offset,std::uint32_t before,std::uint32_t after,
            std::string text,std::locale locale,const std::num_get<char>* get,
            const std::ctype<char>* character,const std::numpunct<char>* punctuation)
            :selected(std::move(x)),physical_stack(std::move(actual)),source_serial(serial),
             axis84(axis),destination_offset(offset),before_word(before),after_word(after),
             payload(std::move(text)),actual_locale(std::move(locale)),
             num_get(get),ctype(character),numpunct(punctuation){}
        void first_without_callbacks()const{
            require(creator==std::this_thread::get_id()&&num_get&&ctype&&numpunct&&
                &std::use_facet<std::num_get<char>>(actual_locale)==num_get&&
                &std::use_facet<std::ctype<char>>(actual_locale)==ctype&&
                &std::use_facet<std::numpunct<char>>(actual_locale)==numpunct,
                "Stack49A030 PRESENT return lost actual locale/facet custody");
        }
    };
    std::unique_ptr<const StackPaddingReadReturn> stack_padding_read_return;
    std::weak_ptr<const NotificationLayoutControlID58> stack_id58_return;
    //Closed Stack685AA0 caller retains the REAL pre-write operands. This is
    //an execution association, never a factory/type or readiness projection.
    struct StackParent38Operands final {
        std::weak_ptr<const NotificationLayoutOrdinaryReceiver> parent;
        std::weak_ptr<NotificationLayoutOrdinaryChildren78> children;
        std::uint32_t parent84,child40_before;
        std::uint64_t source_serial,base_sequence;
        std::size_t base_history_size;
    };
    std::optional<StackParent38Operands> stack_parent38_operands;
    //Private execution guard, never an Original offset or a readiness receipt.
    //Only genuine5AAA20 and returning Common below can advance this member.
    enum class ButtonParserProgress {constructor_returned,common_entered,id_lookup_returned,common_returned};
    std::optional<ButtonParserProgress> button_parser_progress;
    std::uint8_t button_void_missing_read_mask=0; //OWN executed MISS-reader guard; not nativeF4/F8 values.
    //Physical F4/F8. UNKNOWN for every other family; only real Button VALUE ctor engages literal0.
    const std::optional<std::array<std::uint32_t,2>> button_void_constructor_f4_f8;
    std::optional<std::array<std::uint32_t,2>> button_void_f4_f8;
    //Only the actual4A9BC0 member advances these current-word associations.
    //Constructor and both Common MISS-reader observations remain immutable.
    std::uint64_t button_void1_4a9bc0_store_count=0;
    std::optional<std::uint32_t> button_void1_4a9bc0_value;
    //Actual548490 constructs seven NIL LuaRefs84..B4 on the SAME Button.
    //Other subtype allocations never engage this physical field collection.
    std::optional<std::array<std::shared_ptr<const void>,7>> button_callbacks_84_b4;
    bool button_void_current_matches_owned()const noexcept{
        if(!button_void_constructor_f4_f8||!button_void_f4_f8||
            (*button_void_constructor_f4_f8)[0]!=0u||(*button_void_constructor_f4_f8)[1]!=0u||
            (*button_void_f4_f8)[1]!=(*button_void_constructor_f4_f8)[1])return false;
        return button_void1_4a9bc0_store_count?
            (button_void1_4a9bc0_value&&(*button_void_f4_f8)[0]==*button_void1_4a9bc0_value):
            (!button_void1_4a9bc0_value&&*button_void_f4_f8==*button_void_constructor_f4_f8);
    }
    //UNKNOWN until actual548513 writes the SAME original Button receiver.
    std::optional<std::uint32_t> button_state104;
    std::uint64_t button_state104_store_count=0;
    enum class ButtonState104Writer {literal_ctor548513,state_setter6a9610};
    std::optional<ButtonState104Writer> button_state104_writer;
    std::weak_ptr<const NotificationLayoutControlID58> button_id58_return;
    std::weak_ptr<NotificationLayoutEmbeddedLabelState> button_label_return;
    std::weak_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> button_label_factory_return;
    const std::uint64_t id,serial;
    std::weak_ptr<const NotificationLayoutOrdinaryReceiver> canonical;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& c,
        const std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence>& o,
        const std::shared_ptr<NotificationLayoutImageState>& value,std::uint64_t s)
        :core(c),occurrence(o),image(value),id(o->id),serial(s){}
    //Grid-only VALUE snapshots: no borrowed occurrence dereference inside allocator.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::shared_ptr<NotificationLayoutGridState> value,std::uint64_t actual_id,std::uint64_t source_serial)
        :core(std::move(c)),occurrence(std::move(o)),grid(std::move(value)),id(actual_id),serial(source_serial){}
    //Closed Box issuer supplies immutable owned VALUES; no raw borrower read in initializer.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::shared_ptr<NotificationLayoutBoxState> value,std::uint64_t actual_id,std::uint64_t s)
        :core(std::move(c)),occurrence(std::move(o)),box(std::move(value)),id(actual_id),serial(s){}
    //Distinct physical standalone allocation; never inferred from tag/kind/id.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::shared_ptr<NotificationLayoutStandaloneLabelState> value,std::uint64_t actual_id,std::uint64_t s)
        :core(std::move(c)),occurrence(std::move(o)),standalone_label(std::move(value)),id(actual_id),serial(s){}
    //Distinct physical5AAA20 Button; no Image/Box token or tag projection.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::shared_ptr<NotificationLayoutButtonState> value,std::uint64_t actual_id,std::uint64_t s)
        :core(std::move(c)),occurrence(std::move(o)),button(std::move(value)),
         button_parser_progress(ButtonParserProgress::constructor_returned),
         button_void_constructor_f4_f8(std::array<std::uint32_t,2>{0u,0u}),
         button_void_f4_f8(std::array<std::uint32_t,2>{0u,0u}),
         button_callbacks_84_b4(std::in_place),id(actual_id),serial(s){}
    //Stack-only alias is minted from SAMEactual Core + real embedded4BDF90 object.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::shared_ptr<const void> actual_stack,std::uint64_t actual_id,std::uint64_t s)
        :core(std::move(c)),occurrence(std::move(o)),stack(std::move(actual_stack)),
         stack_parser_progress(StackParserProgress::constructor_returned),
         stack_padding_reader_progress(StackPaddingReaderProgress::unentered),id(actual_id),serial(s){}
    // Closed constructor projection snapshots owned operands VALUE before allocation.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> value,std::uint64_t s)
        :core(std::move(c)),occurrence(std::move(o)),context(std::move(value)),id(0),serial(s){}
};
struct detail::NotificationLayoutOrdinaryChildren78::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const NotificationLayoutOrdinaryReceiver> parent;
    std::vector<std::shared_ptr<const NotificationLayoutOrdinaryReceiver>> ordered;
    std::optional<std::uint8_t> native_storage_byte20; // unknown until actual44BF6C writes0; no constructor default claim
    //Stack-only supplier-owned typed port of the real list/vector, not a
    //std::vector.capacity receipt. Every live data slot owns the actual child
    //receiver; inline4/heap/data/count/capacity/change are physical storage.
    //Other families keep their existing ordered collection and have no list.
    struct StackList final {
        using Slot=std::shared_ptr<const NotificationLayoutOrdinaryReceiver>;
        enum class Phase {stable,capacity_writes,allocated,copied,old_released};
        enum class ReserveProgress {unentered,returned};
        const std::thread::id creator=std::this_thread::get_id();
        std::array<Slot,4> inline_slots{};
        std::unique_ptr<Slot[]> heap;
        Slot* data=inline_slots.data();
        std::uint32_t count=0,capacity=4,heap_capacity=0;
        //495E10 does NOT initialize the original list+20 byte. UNKNOWN until
        //a real5B45D0 store1 or44BF40 store0; do not manufacture ctorzero.
        std::optional<std::uint8_t> change;
        Phase phase=Phase::stable;
        ReserveProgress reserve_progress=ReserveProgress::unentered;
        bool failed=false;
        void first_without_callbacks()const{
            require(creator==std::this_thread::get_id()&&!failed&&data,
                "Stack actual495E10 list creator/storage failure");
            if(phase==Phase::old_released){
                //The original frees old noninline storage before installing
                //the new data pointer. Never dereference that old pointer.
                require(!heap&&heap_capacity&&count<=heap_capacity,
                    "Stack5B465F actual freed-old phase changed");
                return;
            }
            require(heap?(data==heap.get()&&heap_capacity>=4&&count<=heap_capacity):
                (data==inline_slots.data()&&!heap_capacity&&count<=4),
                "Stack actual list data/inline/heap physical custody changed");
            if(phase==Phase::stable)require(heap?capacity==heap_capacity:capacity==4,
                "Stack actual stable data/capacity association changed");
        }
    };
    std::unique_ptr<StackList> stack_list;
    //Only the actual Stack Common issuer supplies this already constructed
    //list allocation; all constructor operands are owned VALUES.
    Impl(std::shared_ptr<const NotificationLayoutOrdinaryReceiver> p,
        std::unique_ptr<StackList> actual):parent(std::move(p)),stack_list(std::move(actual)){}
    explicit Impl(const std::shared_ptr<const NotificationLayoutOrdinaryReceiver>& p):parent(p){}
};
struct detail::NotificationLayoutCommonChildFactoryReturn::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    const std::weak_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const std::weak_ptr<NotificationLayoutOrdinaryChildren78> children;
    //Immutable SAME constructor allocation wrapper custody, independent of
    //the mutable Base78 pointer and the released physical Impl/storage.
    const std::shared_ptr<NotificationLayoutOrdinaryChildren78> constructor_children;
    const std::uint64_t serial;
    const std::size_t count;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& c,
        const std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence>& o,
        const std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence>& x,
        const std::shared_ptr<const NotificationLayoutOrdinaryReceiver>& r,
        const std::shared_ptr<NotificationLayoutOrdinaryChildren78>& a,std::uint64_t s,std::size_t n)
        :core(c),occurrence(o),selected(x),receiver(r),children(a),constructor_children(a),serial(s),count(n){}
};
detail::NotificationLayoutOrdinaryReceiver::~NotificationLayoutOrdinaryReceiver()=default;
detail::NotificationLayoutOrdinaryChildren78::~NotificationLayoutOrdinaryChildren78()=default;
detail::NotificationLayoutCommonChildFactoryReturn::~NotificationLayoutCommonChildFactoryReturn()=default;

//Source28: immutable Common children association identifies the constructor
//allocation. Mutable Base78 alone identifies the current pointer-list field.
void BnwWorldViewControlTree::require_native_ordinary_current_children_without_callbacks(
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& receiver,
 const std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn>& returned,
 const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78>& current){
 require(receiver&&receiver->impl_&&returned&&returned->impl_,
  "Current ordinary78 needs its genuine constructor/Common return");
 const auto done=returned->impl_;const auto original=done->children.lock();
 require(done->creator==std::this_thread::get_id()&&done->receiver.lock()==receiver&&original&&
  done->constructor_children==original&&!done->constructor_children.owner_before(original)&&
  !original.owner_before(done->constructor_children),
  "Current ordinary78 lost the SAME constructor list allocation");
 if(current){
  require(current==original&&current->impl_&&current->impl_->creator==done->creator&&
   current->impl_->parent.lock()==receiver,
   "Current ordinary78 differs from its live physical list storage");
 }else require(!original->impl_,
  "Current NULL78 requires actual SAME listv0 storage retirement");
}
