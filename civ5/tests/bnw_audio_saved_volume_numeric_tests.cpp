#include "civ5/ui/bnw_audio_saved_volume_numeric.hpp"
#include <array>
#include <climits>

namespace {
void require(bool condition) {if(!condition)throw std::runtime_error("BNW audio numeric expectation failed");}
struct Expectation {std::int32_t percent; std::uint32_t bits; std::int32_t saved;};
// Fixed source-instruction-derived exact-rational binary32 oracle.
constexpr std::array<Expectation, 101> expected{{
    {0, 0x00000000u, 0},
    {1, 0x3c23d70au, 1},
    {2, 0x3ca3d70au, 2},
    {3, 0x3cf5c28fu, 3},
    {4, 0x3d23d70au, 4},
    {5, 0x3d4cccccu, 4},
    {6, 0x3d75c28fu, 6},
    {7, 0x3d8f5c29u, 7},
    {8, 0x3da3d70au, 8},
    {9, 0x3db851ebu, 9},
    {10, 0x3dccccccu, 9},
    {11, 0x3de147aeu, 11},
    {12, 0x3df5c28fu, 12},
    {13, 0x3e051eb8u, 13},
    {14, 0x3e0f5c29u, 14},
    {15, 0x3e199999u, 14},
    {16, 0x3e23d70au, 16},
    {17, 0x3e2e147bu, 17},
    {18, 0x3e3851ebu, 18},
    {19, 0x3e428f5cu, 19},
    {20, 0x3e4cccccu, 19},
    {21, 0x3e570a3du, 21},
    {22, 0x3e6147aeu, 22},
    {23, 0x3e6b851eu, 22},
    {24, 0x3e75c28fu, 24},
    {25, 0x3e800000u, 25},
    {26, 0x3e851eb8u, 26},
    {27, 0x3e8a3d70u, 26},
    {28, 0x3e8f5c29u, 28},
    {29, 0x3e947ae1u, 29},
    {30, 0x3e999999u, 29},
    {31, 0x3e9eb852u, 31},
    {32, 0x3ea3d70au, 32},
    {33, 0x3ea8f5c2u, 33},
    {34, 0x3eae147bu, 34},
    {35, 0x3eb33333u, 35},
    {36, 0x3eb851ebu, 36},
    {37, 0x3ebd70a4u, 37},
    {38, 0x3ec28f5cu, 38},
    {39, 0x3ec7ae14u, 39},
    {40, 0x3eccccccu, 39},
    {41, 0x3ed1eb85u, 41},
    {42, 0x3ed70a3du, 42},
    {43, 0x3edc28f5u, 42},
    {44, 0x3ee147aeu, 44},
    {45, 0x3ee66666u, 45},
    {46, 0x3eeb851eu, 45},
    {47, 0x3ef0a3d7u, 47},
    {48, 0x3ef5c28fu, 48},
    {49, 0x3efae147u, 48},
    {50, 0x3f000000u, 50},
    {51, 0x3f028f5cu, 51},
    {52, 0x3f051eb8u, 52},
    {53, 0x3f07ae14u, 52},
    {54, 0x3f0a3d70u, 53},
    {55, 0x3f0ccccdu, 55},
    {56, 0x3f0f5c29u, 56},
    {57, 0x3f11eb85u, 57},
    {58, 0x3f147ae1u, 58},
    {59, 0x3f170a3du, 58},
    {60, 0x3f199999u, 59},
    {61, 0x3f1c28f6u, 61},
    {62, 0x3f1eb852u, 62},
    {63, 0x3f2147aeu, 63},
    {64, 0x3f23d70au, 64},
    {65, 0x3f266666u, 65},
    {66, 0x3f28f5c2u, 66},
    {67, 0x3f2b851eu, 66},
    {68, 0x3f2e147bu, 68},
    {69, 0x3f30a3d7u, 69},
    {70, 0x3f333333u, 70},
    {71, 0x3f35c28fu, 71},
    {72, 0x3f3851ebu, 72},
    {73, 0x3f3ae147u, 72},
    {74, 0x3f3d70a4u, 74},
    {75, 0x3f400000u, 75},
    {76, 0x3f428f5cu, 76},
    {77, 0x3f451eb8u, 77},
    {78, 0x3f47ae14u, 78},
    {79, 0x3f4a3d70u, 79},
    {80, 0x3f4cccccu, 79},
    {81, 0x3f4f5c29u, 81},
    {82, 0x3f51eb85u, 82},
    {83, 0x3f547ae1u, 83},
    {84, 0x3f570a3du, 84},
    {85, 0x3f599999u, 85},
    {86, 0x3f5c28f5u, 85},
    {87, 0x3f5eb852u, 87},
    {88, 0x3f6147aeu, 88},
    {89, 0x3f63d70au, 89},
    {90, 0x3f666666u, 90},
    {91, 0x3f68f5c2u, 91},
    {92, 0x3f6b851eu, 91},
    {93, 0x3f6e147bu, 93},
    {94, 0x3f70a3d7u, 94},
    {95, 0x3f733333u, 95},
    {96, 0x3f75c28fu, 96},
    {97, 0x3f7851ebu, 97},
    {98, 0x3f7ae147u, 97},
    {99, 0x3f7d70a3u, 98},
    {100, 0x3f800000u, 100},
}};
template<class F> void rejects(F fn) {
    bool failed=false;
    try {fn();} catch(const std::invalid_argument&) {failed=true;}
    require(failed);
}
}

#if defined(_M_X64) || defined(__x86_64__)
struct FloatingEnvironmentRestore {
    std::fenv_t environment{};
    unsigned int mxcsr = _mm_getcsr();
    FloatingEnvironmentRestore() { require(std::fegetenv(&environment) == 0); }
    ~FloatingEnvironmentRestore() {
        std::fesetenv(&environment);
        _mm_setcsr(mxcsr);
    }
};
#endif

int main() {
    using namespace civ5::ui::audio_settings_detail;
    // Test every forbidden control bit with integer values only. Never actually
    // clear an exception mask in the process floating-point environment.
    require(saved_volume_mxcsr_supported(0x1f80u));
    require(saved_volume_mxcsr_supported(0x1fbfu));
    for (unsigned bit = 6; bit <= 15; ++bit) {
        require(!saved_volume_mxcsr_supported(0x1f80u ^ (1u << bit)));
    }
#if defined(_M_X64) || defined(__x86_64__)
    FloatingEnvironmentRestore restore;
    require(std::fesetround(FE_TONEAREST) == 0);
    _mm_setcsr(0x1f80u);
    for (const auto& r : expected) {
        const auto knob = knob_from_saved_volume_percent(r.percent);
        require(std::bit_cast<std::uint32_t>(knob) == r.bits);
        require(saved_volume_percent_from_knob(knob) == r.saved);
    }
    require(knob_from_saved_volume_percent(INT_MIN) == 0.0f);
    require(knob_from_saved_volume_percent(-1) == 0.0f);
    require(knob_from_saved_volume_percent(101) == 1.0f);
    require(knob_from_saved_volume_percent(INT_MAX) == 1.0f);
    require(saved_volume_percent_from_knob(knob_from_saved_volume_percent(29)) == 29);
    require(saved_volume_percent_from_knob(knob_from_saved_volume_percent(53)) == 52);
    require(saved_volume_percent_from_knob(0.299f) == 29);
    require(saved_volume_percent_from_knob(0.75f) == 75);
    require(saved_volume_percent_from_knob(-0.0f) == 0);
    for (const auto bits : {0x00000001u, 0x007fffffu}) {
        require(saved_volume_percent_from_knob(std::bit_cast<float>(bits)) == 0);
    }
    for (const auto bits : {0x80000001u, 0x807fffffu, 0xbf800000u,
                           0x3f800001u, 0x7f800000u, 0xff800000u,
                           0x7fc00000u, 0x7f800001u}) {
        rejects([bits] { saved_volume_percent_from_knob(std::bit_cast<float>(bits)); });
    }
    // Real environment tests only enable DAZ/FTZ/change rounding with all
    // exception masks set. Guard rejection must precede float bit admission.
    for (const auto csr : {0x1fc0u, 0x9f80u, 0x9fc0u, 0x3f80u, 0x5f80u, 0x7f80u}) {
        _mm_setcsr(csr);
        rejects([] { knob_from_saved_volume_percent(53); });
        rejects([] { saved_volume_percent_from_knob(0.75f); });
        rejects([] { saved_volume_percent_from_knob(std::bit_cast<float>(0x00000001u)); });
        rejects([] { saved_volume_percent_from_knob(std::bit_cast<float>(0x80000001u)); });
    }
    _mm_setcsr(0x1f80u);
    require(std::fesetround(FE_UPWARD) == 0);
    rejects([] { knob_from_saved_volume_percent(29); });
    rejects([] { saved_volume_percent_from_knob(0.75f); });
    require(std::fesetround(FE_TONEAREST) == 0);
    _mm_setcsr(0x1f80u);
    require(saved_volume_percent_from_knob(0.75f) == 75);
#else
    rejects([] { knob_from_saved_volume_percent(29); });
    rejects([] { saved_volume_percent_from_knob(0.75f); });
#endif
}
