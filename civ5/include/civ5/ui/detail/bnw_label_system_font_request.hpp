#pragma once
#include <memory>
#include <cstdint>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics::detail {class BnwLabelSystemFontAtlasMetadata;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwSystemFontCacheOwner;class BnwWorldViewGridToWorldLoan;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwLabelSystemFontSource;
class BnwLabelSystemFontRequest final {
 friend class NotificationContextGuiPort;
 friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::ui::BnwWorldViewGridToWorldLoan;
 friend class ::civ5::graphics::detail::BnwLabelSystemFontAtlasMetadata;
 struct Impl;const std::shared_ptr<const Impl> impl_;
 class Guard final {
  friend class BnwLabelSystemFontRequest;
  friend class NotificationContextGuiPort;
  friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::detail::BnwLabelSystemFontAtlasMetadata;
  const std::shared_ptr<const Impl> impl_;
  explicit Guard(std::shared_ptr<const Impl>);
  ~Guard()=default;
  Guard(const Guard&)=delete;
  Guard& operator=(const Guard&)=delete;
  void first_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  std::shared_ptr<const BnwLabelSystemFontSource> source_without_callbacks()const;
  std::shared_ptr<BnwSystemFontCacheOwner> cache_without_callbacks()const;
  std::int32_t height_without_callbacks()const;
  std::uint64_t serial_without_callbacks()const;
 };
 Guard capture_guard_without_callbacks()const;
 explicit BnwLabelSystemFontRequest(std::shared_ptr<const Impl>);
 void require_pending_without_callbacks()const;
 void require_pending_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 std::shared_ptr<const BnwLabelSystemFontSource> source_without_callbacks()const;
 std::shared_ptr<BnwSystemFontCacheOwner> cache_without_callbacks()const;
 std::int32_t height_without_callbacks()const;
public:
 ~BnwLabelSystemFontRequest();
 BnwLabelSystemFontRequest(const BnwLabelSystemFontRequest&)=delete;
 BnwLabelSystemFontRequest&operator=(const BnwLabelSystemFontRequest&)=delete;
};
}
