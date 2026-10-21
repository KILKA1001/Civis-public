#include "civ5/ui/bnw_audio_knob_direct_store.hpp"
#include <array>

namespace {
void require(bool ok) { if (!ok) throw std::runtime_error("BNW direct knob expectation failed"); }
struct SurroundingFields {
    float current;
    float target;
    float slope;
    std::int32_t transition_mode;
    float effective;
    std::uint32_t parent_marker;
    std::uint8_t flag;
};
template<class F> void rejects(F fn) {
    bool failed = false;
    try { fn(); } catch (const std::invalid_argument&) { failed = true; }
    require(failed);
}
#if defined(_M_X64) || defined(__x86_64__)
struct EnvironmentRestore {
    std::fenv_t env{};
    unsigned int csr = _mm_getcsr();
    EnvironmentRestore() { require(std::fegetenv(&env) == 0); }
    ~EnvironmentRestore() { std::fesetenv(&env); _mm_setcsr(csr); }
};
#endif
}

int main() {
    using namespace civ5::ui::audio_settings_detail;
#if defined(_M_X64) || defined(__x86_64__)
    EnvironmentRestore restore;
    require(std::fesetround(FE_TONEAREST) == 0);
    _mm_setcsr(0x1f80u);
    struct Case { std::uint32_t input; std::uint32_t output; };
    constexpr std::array<Case, 12> cases{{
        {0x00000000u, 0x00000000u}, {0x80000000u, 0x80000000u},
        {0x00000001u, 0x00000001u}, {0x007fffffu, 0x007fffffu},
        {0x80000001u, 0x00000000u}, {0x807fffffu, 0x00000000u},
        {0xbf800000u, 0x00000000u}, {0xff7fffffu, 0x00000000u},
        {0x3f400000u, 0x3f400000u}, {0x3f800000u, 0x3f800000u},
        {0x3f800001u, 0x3f800000u}, {0x7f7fffffu, 0x3f800000u},
    }};
    for (const auto c : cases) {
        SurroundingFields state{0.4f, 0.2f, -0.3f, -37, 0.6f, 0x11223344u, 0xa5};
        apply_knob_direct_numeric(state.current, state.transition_mode, std::bit_cast<float>(c.input));
        require(std::bit_cast<std::uint32_t>(state.current) == c.output);
        require(state.transition_mode == 0);
        require(state.target == 0.2f && state.slope == -0.3f && state.effective == 0.6f);
        require(state.parent_marker == 0x11223344u && state.flag == 0xa5);
    }
    float current = 0.4f;
    std::int32_t mode = 37;
    for (const auto bits : {0x7f800000u, 0xff800000u, 0x7fc00000u, 0x7f800001u}) {
        rejects([&] { apply_knob_direct_numeric(current, mode, std::bit_cast<float>(bits)); });
        require(current == 0.4f && mode == 37);
    }
    _mm_setcsr(0x1fc0u); // DAZ with all masks enabled; guard must reject before stores.
    rejects([&] { apply_knob_direct_numeric(current, mode, std::bit_cast<float>(0x80000001u)); });
    _mm_setcsr(0x1f80u);
    require(current == 0.4f && mode == 37);
#else
    float current = 0.4f;
    std::int32_t mode = 37;
    rejects([&] { apply_knob_direct_numeric(current, mode, 0.75f); });
    require(current == 0.4f && mode == 37);
#endif
}
