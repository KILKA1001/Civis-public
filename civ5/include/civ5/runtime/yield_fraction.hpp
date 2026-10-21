#pragma once
#include <cstdint>

namespace civ5::runtime {
// Exact yield fractions. Normalize before checking the 64-bit stored result;
// the original's overflowing int32 products are not part of the game rules.
class YieldFraction {
  public:
    YieldFraction(std::int64_t numerator=0,std::int64_t denominator=1);
    std::int64_t numerator() const { return numerator_; }
    std::int64_t denominator() const { return denominator_; }
    std::int64_t truncate() const { return numerator_/denominator_; }
    std::int64_t scaled_truncate(std::int64_t multiplier,std::int64_t scale=1) const;
    YieldFraction &operator+=(const YieldFraction &rhs);
    bool operator==(const YieldFraction &) const = default;
  private:
    std::int64_t numerator_=0,denominator_=1;
};
} // namespace civ5::runtime
