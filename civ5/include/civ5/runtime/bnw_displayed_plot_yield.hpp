#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
namespace civ5::runtime {
class LiveSession;
class BnwNormalPlotYieldRules;
class BnwDisplayedPlotYieldUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// Retained genuine synchronous numeric query owner. Each call reads the
// CURRENT guarded session; it is not an immutable yield quote or UI delivery.
class BnwDisplayedPlotYield final {
 public:
  std::int32_t value()const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwDisplayedPlotYield(std::shared_ptr<Impl>)noexcept;
  void bind_initial();
  void require_current()const;
  // Defined beside the existing normal formula, sharing one private body.
  // The request is always display=true; Game debug is read from LiveSession.
  static std::int32_t calculate_source(const LiveSession&,const BnwNormalPlotYieldRules&,
                                     std::int32_t plot,std::int32_t yield);
  friend BnwDisplayedPlotYield inspect_bnw_displayed_plot_yield(
   std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwNormalPlotYieldRules>,
   std::int64_t plot,std::int64_t yield);
};
BnwDisplayedPlotYield inspect_bnw_displayed_plot_yield(
 std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwNormalPlotYieldRules>,
 std::int64_t plot,std::int64_t yield);
}
