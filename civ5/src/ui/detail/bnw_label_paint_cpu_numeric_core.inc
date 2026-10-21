// Root-private numeric DATA primitives. Only the actual sealed Paint producer
// supplies the genuine Frame profile. No public diagnostic bool/profile issuer.
namespace detail {
class BnwLabelPaintCpuNumeric final {
    friend class BnwLabelPaintTextLoan;
    static void profile(const std::array<std::uint32_t,2>& actual){
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
        std::uint16_t current_cw=0;std::uint32_t current_mx=0;
        __asm__ volatile("fnstcw %0\n\tstmxcsr %1":"=m"(current_cw),"=m"(current_mx));
        require(actual[0]==current_cw&&(actual[1]&~std::uint32_t{0x3f})==
            (current_mx&~std::uint32_t{0x3f}),"Paint CPU genuine numeric control profile changed");
        //Unmasked/native trapping behavior is not admitted by this small
        //normalized producer. No success/defaults are supplied for that path.
        require((current_cw&0x3fu)==0x3fu&&((current_cw>>8)&3u)!=1,
            "Paint CPU unmasked/reserved x87 profile requires separate native exception owner");
#else
        (void)actual;throw std::runtime_error("Paint CPU source numeric primitives need supported x87 host/compiler");
#endif
    }
    static std::uint32_t advance_low32(std::uint32_t native_float_bits,
        const std::array<std::uint32_t,2>& actual){
        profile(actual);
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
        std::uint16_t saved_cw=0;
        const auto truncated_cw=static_cast<std::uint16_t>(actual[0]|0x0c00u);
        std::int64_t value=0;
        //6C6055/591D88: actual float32 FLD, save real CW, force only RC,
        //FISTP64, consume low32, restore original CW. Native masked-invalid
        //indefinite naturally has low32 zero; no finite/int32 clamp is added.
        __asm__ volatile("flds %2\n\tfnstcw %1\n\tfldcw %3\n\tfistpll %0\n\tfldcw %1"
            :"=&m"(value),"=&m"(saved_cw)
            :"m"(native_float_bits),"m"(truncated_cw):"memory","st");
        profile(actual);return static_cast<std::uint32_t>(value);
#else
        (void)native_float_bits;throw std::runtime_error("Paint CPU advance needs supported x87 host/compiler");
#endif
    }
    static std::uint32_t newline_height_low32(std::uint32_t actual_font_40c10,
        const std::array<std::uint32_t,2>& actual){
        profile(actual);
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
        const auto signed_metric=std::bit_cast<std::int32_t>(actual_font_40c10);
        constexpr std::uint32_t two32=0x4f800000u; //Original PE9DD390 exact bits.
        constexpr std::uint32_t scale=0x3f99999au; //Original PE9E1548 exact bits.
        std::uint16_t saved_cw=0;
        const auto truncated_cw=static_cast<std::uint16_t>(actual[0]|0x0c00u);
        std::int64_t value=0;
        //Original4C4DA0 unsigned FILD correction precedes FMUL1.2(float32),
        //then the same RC-truncate FISTP64 lowword and saved-CW restoration.
        if(actual_font_40c10&0x80000000u){
            __asm__ volatile("fildl %2\n\tfadds %3\n\tfmuls %4\n\tfnstcw %1\n\tfldcw %5\n\tfistpll %0\n\tfldcw %1"
                :"=&m"(value),"=&m"(saved_cw):"m"(signed_metric),"m"(two32),"m"(scale),"m"(truncated_cw)
                :"memory","st");
        }else{
            __asm__ volatile("fildl %2\n\tfmuls %3\n\tfnstcw %1\n\tfldcw %4\n\tfistpll %0\n\tfldcw %1"
                :"=&m"(value),"=&m"(saved_cw):"m"(signed_metric),"m"(scale),"m"(truncated_cw)
                :"memory","st");
        }
        profile(actual);return static_cast<std::uint32_t>(value);
#else
        (void)actual_font_40c10;throw std::runtime_error("Paint CPU newline needs supported x87 host/compiler");
#endif
    }
};
}
