#include "civ5/runtime/yield_fraction.hpp"
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
// Supported x64 clang/GCC targets supply a signed 128-bit intermediate.
// Two signed 64-bit products and their sum fit; stored values remain int64.
using Intermediate=__int128;
std::pair<std::int64_t,std::int64_t> normalize(Intermediate n,Intermediate d){
    if(!d)throw std::invalid_argument("Zero yield fraction denominator");
    if(d<0){n=-n;d=-d;}
    auto a=n<0?-n:n,b=d;
    while(b){const auto next=a%b;a=b;b=next;}
    n/=a;d/=a;
    if(n<INT64_MIN||n>INT64_MAX||d>INT64_MAX)throw std::overflow_error("Reduced yield fraction exceeds int64");
    return {static_cast<std::int64_t>(n),static_cast<std::int64_t>(d)};
}
}
YieldFraction::YieldFraction(std::int64_t n,std::int64_t d){
    const auto value=normalize(n,d);numerator_=value.first;denominator_=value.second;
}
YieldFraction &YieldFraction::operator+=(const YieldFraction &rhs){
    const auto value=normalize(Intermediate(numerator_)*rhs.denominator_+Intermediate(rhs.numerator_)*denominator_,
        Intermediate(denominator_)*rhs.denominator_);
    numerator_=value.first;denominator_=value.second;return *this;
}
std::int64_t YieldFraction::scaled_truncate(std::int64_t multiplier,std::int64_t scale)const{
    using U=unsigned __int128;
    const auto magnitude=[](std::int64_t v){const auto u=static_cast<std::uint64_t>(v);return v<0?std::uint64_t{0}-u:u;};
    const U product=U(magnitude(numerator_))*magnitude(multiplier);const U factor=magnitude(scale);
    // A product wider than 128 bits cannot fit int64 after division by the
    // positive int64 denominator. Do not overflow before diagnosing it.
    if(factor&&product>(~U{0})/factor)throw std::overflow_error("Scaled yield fraction exceeds int64");
    const U quotient=product*factor/static_cast<std::uint64_t>(denominator_);
    const bool negative=(numerator_<0)^(multiplier<0)^(scale<0);const U bound=U(INT64_MAX)+(negative?1:0);
    if(quotient>bound)throw std::overflow_error("Scaled yield fraction exceeds int64");
    if(negative&&quotient==U(INT64_MAX)+1)return INT64_MIN;
    const auto n=static_cast<std::int64_t>(quotient);return negative?-n:n;
}
} // namespace civ5::runtime
