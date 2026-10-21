// Inside civ5::ui. Native semantic manager+4 key map, not an x86 ABI struct.
struct detail::BnwLabelSystemFontRegistryFields final {
 struct Life final {
  const std::thread::id creator=std::this_thread::get_id();
  std::atomic<bool> alive{true},failed{false};
  void first()const{
   require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"Gui+10 system font cache retired/foreign/failed");
  }
 };
 struct Entry final {
  const std::shared_ptr<Life> life=std::make_shared<Life>();
  const std::int32_t height;
  const std::uint64_t occurrence;
  std::shared_ptr<const detail::BnwLabelSystemFontSource> source;
  std::shared_ptr<BnwSystemFontCpuOwner> cpu;
  std::shared_ptr<BnwSystemFontCacheOwner> cache;
  std::weak_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> atlas;
  bool returned=false;
  bool text_operation_active=false;
  std::uint64_t preparation_sequence=0;
  Entry(std::int32_t h,std::uint64_t serial):height(h),occurrence(serial){}
  ~Entry(){life->alive.store(false);}
 };
 struct Slot final {std::shared_ptr<Entry> completed;};
 const std::shared_ptr<Life> life=std::make_shared<Life>();
 std::map<std::int32_t,Slot> map;
 std::vector<std::shared_ptr<Entry>> pending;
 std::uint64_t sequence=0;
 bool busy=false;
 ~BnwLabelSystemFontRegistryFields(){life->alive.store(false);}
};
