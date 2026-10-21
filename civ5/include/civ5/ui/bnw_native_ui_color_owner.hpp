#pragma once
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include <array>
#include <map>
namespace civ5::ui {
using BnwNativeColor=std::array<float,4>;
struct BnwNativeColorContextRead {
 std::shared_ptr<const void> identity;
 std::array<std::optional<bool>,2> enabled;
 // A present map is the owned complete source map, not a grep inventory.
 // Missing enabled/map states are mandatory dependencies, not empty maps.
 std::array<std::optional<std::map<std::uint32_t,BnwNativeColor>>,2> maps;
 bool operator==(const BnwNativeColorContextRead&)const=default;
};
struct BnwNativeColorUnresolvedProvider {
 // Dependency identity, NOT an instantiated native context or stack position.
 std::shared_ptr<const void> identity;
 std::string source;
 bool operator==(const BnwNativeColorUnresolvedProvider&)const=default;
};
struct BnwNativeColorRegistryRead {
 std::shared_ptr<const void> identity;
 std::uint64_t revision=0;
 // Native stack order: oldest first; lookup visits reverse order.
 std::vector<BnwNativeColorContextRead> contexts;
 // Final global map needs actual base ColorAtlas + Rules cache load order.
 // Rules rows alone cannot certify a global miss.
 std::optional<std::map<std::uint32_t,BnwNativeColor>> global_map;
 // An unresolved provider can precede any known hit. No total native stack
 // order/absence may be inferred from a partial DLC projection.
 std::optional<BnwNativeColorUnresolvedProvider> unresolved_provider;
 bool operator==(const BnwNativeColorRegistryRead&)const=default;
};
struct BnwNativeColorRegistryProvider {
 std::shared_ptr<const void> owner;
 std::function<std::optional<BnwNativeColorRegistryRead>()> current;
};
struct BnwNativeGlobalColor {
 std::int64_t id=0;
 std::string type;
 std::uint32_t hash=0;
 BnwNativeColor rgba;
 bool operator==(const BnwNativeGlobalColor&)const=default;
};
enum class BnwNativeColorRoute { context,global,numeric };
class BnwNativeUiColorOwner;
class BnwNativeUiColorQuote {
 public:
 const BnwNativeColor&rgba()const;
 BnwNativeColorRoute route()const;
 private:
 friend class BnwNativeUiColorOwner;
 struct Data;
 std::shared_ptr<const Data> data_;
 explicit BnwNativeUiColorQuote(std::shared_ptr<const Data>);
};
// Retains actual selected rules Colors and current-process client identity.
// This owner cannot create or certify a stock context stack: missing producer
// stays unavailable. Registry fixtures are not stock loading receipts.
class BnwNativeUiColorOwner {
 public:
 static std::shared_ptr<const BnwNativeUiColorOwner> create_from_adopted_host(
  std::shared_ptr<const SelectedBnwPreviewHost>);
 static std::shared_ptr<const BnwNativeUiColorOwner> create(
  std::shared_ptr<const SelectedBnwPreviewHost>,std::uintptr_t actual_hwnd,
  runtime::BnwStockLuaClientExtentProvider,BnwNativeColorRegistryProvider={});
 ~BnwNativeUiColorOwner();
 const std::vector<BnwNativeGlobalColor>&global_colors()const;
 const saves::SaveHeader&selected_header()const;
 const runtime::BnwStockLuaClientExtent&client_extent()const;
 BnwNativeUiColorQuote quote(std::string_view declaration)const;
 void validate_quote(const BnwNativeUiColorQuote&)const;
 private:
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit BnwNativeUiColorOwner(std::unique_ptr<Impl>);
};
}
