#pragma once
#include "civ5/ui/bnw_native_ui_color_owner.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/file_identity.hpp"
namespace civ5::ui {
struct BnwNativeColorAtlasRead {
 std::map<std::uint32_t,BnwNativeColor> colors;
 std::size_t color_rows=0,color_sets=0;
};
// Native601CA0 Color map projection only. ColorSet is a separate native map;
// neither this parser nor this projection certifies its consumer/init.
BnwNativeColorAtlasRead parse_bnw_native_color_atlas(const content::XmlNode&);
struct BnwSelectedColorProjectionFrame {
 std::size_t selected_packages=0,skin_contexts=0,slot_maps=0;
 std::size_t base_color_rows=0,base_color_sets=0,skin_color_sets=0;
 // Describes selected DLC maps only, never all possible native providers.
 bool all_skin_color_maps_empty=false,global_hashes_compatible=false;
 bool potential_registry_complete=false;
 bool owned_global_ui_then_rules=false;
 bool native_skin_loading_complete=false,stock_loading_complete=false;
 std::vector<std::string> source_order;
};
class BnwSelectedNativeColorProjection {
 public:
 static std::shared_ptr<const BnwSelectedNativeColorProjection> create(
  const std::filesystem::path&shared_root,const content::Catalog&,
  const runtime::SavedContentProfile&,
  std::shared_ptr<const BnwNativeUiColorOwner>actual_rules_stage);
 ~BnwSelectedNativeColorProjection();
 const BnwSelectedColorProjectionFrame&current()const;
 BnwNativeColorRegistryProvider provider()const;
 private:
 struct Impl;std::shared_ptr<Impl>impl_;
 explicit BnwSelectedNativeColorProjection(std::shared_ptr<Impl>);
};
}
