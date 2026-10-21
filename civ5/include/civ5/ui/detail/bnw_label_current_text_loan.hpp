#pragma once
#include <memory>
#include <string_view>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui::detail {
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
class NotificationContextGuiPort;
class BnwLabelSystemFontBorrower;
class BnwLabelSystemFontPreparedRun;
// Actual lexical C4 input for the closed Label initialization producers.
// Retaining a physical UTF8 copy is never authority after its issuing frame.
class BnwLabelCurrentTextLoan final {
 friend class NotificationLayoutEmbeddedLabelState;
 friend class NotificationLayoutStandaloneLabelState;
 friend class NotificationContextGuiPort;
 friend class BnwLabelSystemFontBorrower;
 friend class BnwLabelSystemFontPreparedRun;
 enum class Phase {position_7505,size_5077};
 struct StandaloneImpl;
 struct Impl;const std::shared_ptr<Impl> impl_;
 class Guard final {
  friend class BnwLabelCurrentTextLoan;
  friend class NotificationLayoutEmbeddedLabelState;
  friend class NotificationContextGuiPort;
  friend class BnwLabelSystemFontBorrower;
  friend class BnwLabelSystemFontPreparedRun;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks()const;
  void require_system_font_borrower_without_callbacks(const BnwLabelSystemFontBorrower&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  std::string_view actual_utf8_without_callbacks()const;
  std::string_view actual_c4_bytes_without_callbacks()const;
  Phase phase_without_callbacks()const;
 };
 Guard capture_guard_without_callbacks()const;
 explicit BnwLabelCurrentTextLoan(std::shared_ptr<Impl>);
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 std::string_view actual_utf8_without_callbacks()const;
 std::string_view actual_c4_bytes_without_callbacks()const;
 Phase phase_without_callbacks()const;
public:
 ~BnwLabelCurrentTextLoan();
 BnwLabelCurrentTextLoan(const BnwLabelCurrentTextLoan&)=delete;
 BnwLabelCurrentTextLoan&operator=(const BnwLabelCurrentTextLoan&)=delete;
};
}
