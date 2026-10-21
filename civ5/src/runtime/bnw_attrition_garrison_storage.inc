// In existing Core anonymous namespace AFTER damage storage and before Impl.
struct AttritionGarrisonChild {
 std::uint64_t serial=0,previous=0,revision=0;
 bool prepared=false,executed=false,failed=false;
 std::uint32_t entered=0,returned=0;
 std::array<std::shared_ptr<const BnwNativeImportedGarrisonRead>,3>reads;
 BnwAttritionGarrisonBoundary boundary=BnwAttritionGarrisonBoundary::query_owner;
};
