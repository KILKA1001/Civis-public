#pragma once
#include <cstdint>
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwSystemFontCpuOwner;struct BnwSystemFontCpuFrame;class UiScreenGlRenderer;}
namespace civ5::graphics::detail {class BnwLabelSystemFontAtlasMetadata;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwLabelSystemFontBorrower;
class BnwLabelSystemFontRequest;
// Genuine Gui-owned signed-height cell source, sealed before its GDI constructor.
// Read-only CPU fields do not issue Font, cache, Label or frame completion.
class BnwLabelSystemFontSource final {
 friend class NotificationContextGuiPort;
 friend class BnwLabelSystemFontBorrower;
 friend class BnwLabelSystemFontRequest;
 friend class ::civ5::ui::BnwSystemFontCpuOwner;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::detail::BnwLabelSystemFontAtlasMetadata;
 struct Impl;const std::shared_ptr<const Impl> impl_;
 explicit BnwLabelSystemFontSource(std::shared_ptr<const Impl>);
 void require_current_without_callbacks()const;
 void require_current_owned()const;
 void require_same_operation_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan&)const;
 BnwSystemFontCpuFrame captured_cpu_frame_owned()const;
 std::uintptr_t hwnd_without_callbacks()const;
public:
 ~BnwLabelSystemFontSource();
 BnwLabelSystemFontSource(const BnwLabelSystemFontSource&)=delete;
 BnwLabelSystemFontSource&operator=(const BnwLabelSystemFontSource&)=delete;
};
}
