#pragma once
#include <memory>
#include <string>
#include <atomic>
#include <cstdint>
#include <thread>
namespace civ5::graphics::detail {class BnwFontTextureObservation;class BnwFontTextureBorrower;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;namespace detail {class BnwFontResourceLoan;}}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwFontTextureRequest final {
 friend class NotificationContextGuiPort;
 friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
 friend class ::civ5::runtime::detail::BnwFontResourceLoan;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::detail::BnwFontTextureObservation;
 friend class ::civ5::graphics::detail::BnwFontTextureBorrower;
 struct Life final {
 private:
  friend class BnwFontTextureRequest;
  friend class ::civ5::graphics::detail::BnwFontTextureBorrower;
  const std::thread::id creator_=std::this_thread::get_id();
  std::atomic<bool> alive_{true};mutable std::atomic<bool> failed_{false};
  const std::uint64_t occurrence_;
  explicit Life(std::uint64_t occurrence):occurrence_(occurrence){}
 public:
  void require_current_owned(std::uint64_t expected_occurrence)const;
 };
 struct Impl;const std::unique_ptr<Impl> impl_;
 const std::shared_ptr<Life> life_;
 explicit BnwFontTextureRequest(std::unique_ptr<Impl>);
 void require_pending_query_owned(const runtime::BnwWorldViewSourceOperationLoan&,const runtime::detail::BnwFontResourceLoan&)const;
 void seal_native_row_append_owned(const runtime::BnwWorldViewSourceOperationLoan&,const runtime::detail::BnwFontResourceLoan&);
 void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,
                            const runtime::detail::BnwFontResourceLoan&)const;
 const std::string& source_owned(const runtime::BnwWorldViewSourceOperationLoan&,
                               const runtime::detail::BnwFontResourceLoan&)const;
 bool optional_index_owned(const runtime::BnwWorldViewSourceOperationLoan&,
                         const runtime::detail::BnwFontResourceLoan&)const;
 std::uint64_t texture_occurrence_owned(const runtime::BnwWorldViewSourceOperationLoan&,
                                      const runtime::detail::BnwFontResourceLoan&)const;
 std::weak_ptr<Life> texture_lifetime_owned(const runtime::BnwWorldViewSourceOperationLoan&,
                                        const runtime::detail::BnwFontResourceLoan&)const;
public:
 BnwFontTextureRequest(const BnwFontTextureRequest&)=delete;
 BnwFontTextureRequest&operator=(const BnwFontTextureRequest&)=delete;
 ~BnwFontTextureRequest();
};
}
